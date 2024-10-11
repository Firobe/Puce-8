open Tsdl
open Result

let exec_fps = 400.
let audio_fps = 40.

(* 0x000 to 0xFFF main memory *)
let memory = Array.make 0x1000 0x00

(* 16 general-purpose registers *)
let general_registers = Array.make 0x10 0x00

(* SP : 8-bit *)
let stack_pointer = ref 0x00

(* Sound registers *)
let delay_timer = ref 0x00
let sound_timer = ref 0x00

(* 16-bit registers *)
let register_I = ref 0x00
let program_counter = ref 0x00

(* Stack : 16 16-bit values *)
let stack = Array.make 0x10 0x00

(* Utils *)

let scale = 10
let width = scale * 64
let height = scale * 32

let keyboard_map =
  [
    Sdl.Scancode.kp_0;
    Sdl.Scancode.kp_7;
    Sdl.Scancode.kp_8;
    Sdl.Scancode.kp_9;
    Sdl.Scancode.kp_4;
    Sdl.Scancode.kp_5;
    Sdl.Scancode.kp_6;
    Sdl.Scancode.kp_1;
    Sdl.Scancode.kp_2;
    Sdl.Scancode.kp_3;
    Sdl.Scancode.right;
    Sdl.Scancode.kp_period;
    Sdl.Scancode.a;
    Sdl.Scancode.z;
    Sdl.Scancode.e;
    Sdl.Scancode.r;
  ]

let _dump_registers () =
  Printf.printf "PC = %.4X   SP = %.2X\n" !program_counter !stack_pointer;
  Printf.printf "DT = %.4X   ST = %.4X\n" !delay_timer !sound_timer;
  Array.iteri (Printf.printf "V%X = %.2X\n") general_registers;
  Printf.printf "RI = %.4X\n%!" !register_I

let stack_read () = Array.get stack !stack_pointer
let stack_pop () = stack_pointer := !stack_pointer - 1

let stack_push v =
  stack_pointer := !stack_pointer + 1;
  Array.set stack !stack_pointer v

let read_register n = Array.get general_registers n
let set_register x v = Array.set general_registers x v

let shift_and_mask v dec mask =
  let target = v lsr (dec * 4) in
  target land mask

let renderer = ref None
let get_renderer () = match !renderer with Some r -> r | None -> assert false

let clear_screen () =
  ignore (Sdl.set_render_draw_color (get_renderer ()) 0 0 0 255);
  ignore (Sdl.render_clear (get_renderer ()))

let screen = Array.make_matrix 64 32 false

let display_byte x y b =
  let b = ref b in
  let acc = ref false in
  let yp = y mod 32 in
  for j = 0 to 7 do
    let i = 7 - j in
    let xp = (x + i) mod 64 in
    let v = !b land 0x1 = 1 in
    let oldv = screen.(xp).(yp) in
    acc := !acc || (oldv && v);
    let rect =
      Sdl.Rect.create ~x:(xp * scale) ~y:(yp * scale) ~w:scale ~h:scale
    in
    let color = if oldv = v then 0 else 255 in
    ignore (Sdl.set_render_draw_color (get_renderer ()) color color color 255);
    ignore (Sdl.render_fill_rect (get_renderer ()) (Some rect));
    screen.(xp).(yp) <- oldv != v;
    b := !b lsr 1
  done;
  !acc

let sprites =
  [
    0xF0;
    0x90;
    0x90;
    0x90;
    0xF0;
    (* 0 *)
    0x20;
    0x60;
    0x20;
    0x20;
    0x70;
    (* 1 *)
    0xF0;
    0x10;
    0xF0;
    0x80;
    0xF0;
    (* 2 *)
    0xF0;
    0x10;
    0xF0;
    0x10;
    0xF0;
    (* 3 *)
    0x90;
    0x90;
    0xF0;
    0x10;
    0x10;
    (* 4 *)
    0xF0;
    0x80;
    0xF0;
    0x10;
    0xF0;
    (* 5 *)
    0xF0;
    0x80;
    0xF0;
    0x90;
    0xF0;
    (* 6 *)
    0xF0;
    0x10;
    0x20;
    0x40;
    0x40;
    (* 7 *)
    0xF0;
    0x90;
    0xF0;
    0x90;
    0xF0;
    (* 8 *)
    0xF0;
    0x90;
    0xF0;
    0x10;
    0xF0;
    (* 9 *)
    0xF0;
    0x90;
    0xF0;
    0x90;
    0x90;
    (* A *)
    0xE0;
    0x90;
    0xE0;
    0x90;
    0xE0;
    (* B *)
    0xF0;
    0x80;
    0x80;
    0x80;
    0xF0;
    (* C *)
    0xE0;
    0x90;
    0x90;
    0x90;
    0xE0;
    (* D *)
    0xF0;
    0x80;
    0xF0;
    0x80;
    0xF0;
    (* E *)
    0xF0;
    0x80;
    0xF0;
    0x80;
    0x80 (* F *);
  ]

(* INSTRUCTION SET *)

let invalid_instruction op =
  Printf.printf "Invalid instruction %.4X at PC = %.4X\n" op !program_counter;
  assert false

let op_table =
  [
    (function
    | 0x00E0 ->
        (* CLS *)
        clear_screen ();
        for x = 0 to 63 do
          Array.fill screen.(x) 0 31 false
        done
    | 0x00EE ->
        (* RET *)
        program_counter := stack_read ();
        stack_pop ()
    | op -> invalid_instruction op);
    (fun (* 1nnn  : JP addr *)
           op ->
      let nnn = shift_and_mask op 0 0xFFF in
      program_counter := nnn);
    (fun (* 2nnn : CALL addr *)
           op ->
      let nnn = shift_and_mask op 0 0xFFF in
      stack_push !program_counter;
      program_counter := nnn);
    (fun (* 3xkk : SE Vx, byte *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let kk = shift_and_mask op 0 0xFF in
      if read_register x = kk then program_counter := !program_counter + 2);
    (fun (* 4xkk : SNE Vx, byte *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let kk = shift_and_mask op 0 0xFF in
      if read_register x != kk then program_counter := !program_counter + 2);
    (fun (* 5xy0 : SE Vx, Vy *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let y = shift_and_mask op 1 0xF in
      if read_register x = read_register y then
        program_counter := !program_counter + 2);
    (fun (* 6xkk : LD Vx, byte *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let kk = shift_and_mask op 0 0xFF in
      set_register x kk);
    (fun (* 7xkk : ADD Vx, byte *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let kk = shift_and_mask op 0 0xFF in
      let sum = kk + read_register x in
      set_register x (0xFF land sum));
    (fun (* 8xy[op] *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let y = shift_and_mask op 1 0xF in
      let rx = read_register x in
      let ry = read_register y in
      let op = shift_and_mask op 0 0xF in
      let res =
        match op with
        | 0x0 ->
            (* LD Vx, Vy *)
            ry
        | 0x1 ->
            (* OR Vx, Vy *)
            rx lor ry
        | 0x2 ->
            (* AND Vx, Vy *)
            rx land ry
        | 0x3 ->
            (* XOR Vx, Vy *)
            rx lxor ry
        | 0x4 ->
            (* ADD Vx, Vy *)
            let sum = rx + ry in
            set_register 0xF (if sum > 0xFF then 1 else 0);
            sum land 0xFF
        | 0x5 ->
            (* SUB Vx, Vy *)
            set_register 0xF (if rx > ry then 1 else 0);
            (rx - ry) land 0xFF
        | 0x6 ->
            (* SHR Vx {, Vy} *)
            let vf = if rx land 0x1 = 0x1 then 1 else 0 in
            set_register 0xF vf;
            rx lsr 1
        | 0x7 ->
            (* SUBN Vx, Vy *)
            set_register 0xF (if ry > rx then 1 else 0);
            (ry - rx) land 0xFF
        | 0xE ->
            (* SHL Vx {, Vy} *)
            let vf = if rx land 0x1 = 0x1 then 1 else 0 in
            set_register 0xF vf;
            rx lsl 1
        | _ -> invalid_instruction op
      in
      set_register x res);
    (fun (* 9xy0 : SNE Vx, Vy *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let y = shift_and_mask op 1 0xF in
      if read_register x != read_register y then
        program_counter := !program_counter + 2);
    (fun (* Annn : LD I, addr *)
           op ->
      let nnn = shift_and_mask op 0 0xFFF in
      register_I := nnn);
    (fun (* Bnnn : JP V0, addr *)
           op ->
      let nnn = shift_and_mask op 0 0xFFF in
      program_counter := read_register 0 + nnn);
    (fun (* Cxkk : RND Vx, byte *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let kk = shift_and_mask op 0 0xFF in
      let rng = Random.int 0x100 in
      (*Printf.printf "generated %.2X\n!" rng;*)
      let v = rng land kk in
      set_register x v);
    (fun (* Dxyn : DRW Vx, Vy, nibble *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let y = shift_and_mask op 1 0xF in
      let n = shift_and_mask op 0 0xF in
      let rx = read_register x in
      let ry = read_register y in
      let acc = ref false in
      for i = 0 to n - 1 do
        let byte = Array.get memory (!register_I + i) in
        acc := display_byte rx (ry + i) byte || !acc
      done;
      set_register 0xF (if !acc then 1 else 0));
    (fun (* Ex(9E|A1) : SKP(N) Vx *)
           op ->
      let x = shift_and_mask op 2 0xF in
      let bol =
        match shift_and_mask op 0 0xFF with
        | 0x9E -> true
        | 0xA1 -> false
        | _ -> invalid_instruction op
      in
      let key_nb = read_register x in
      let key_scancode = List.nth keyboard_map key_nb in
      let state = Sdl.get_keyboard_state () in
      let key_state = Bigarray.Array1.get state key_scancode = 1 in
      let xor = if bol then key_state else not key_state in
      if xor then program_counter := !program_counter + 2);
    (fun op ->
      let x = shift_and_mask op 2 0xF in
      let rx = read_register x in
      match shift_and_mask op 0 0xFF with
      | 0x07 ->
          (* LD Vx, DT *)
          set_register x !delay_timer
      | 0x0A ->
          (* LD Vx, K *)
          (* Wait for input and store value in Vx *)
          let event = Sdl.Event.create () in
          let value = ref 0xFF in
          while !value = 0xFF do
            ignore (Sdl.wait_event (Some event));
            let event_type = Sdl.Event.get event Sdl.Event.typ in
            if event_type = Sdl.Event.key_down then
              let scancode = Sdl.Event.get event Sdl.Event.keyboard_scancode in
              List.iteri
                (fun i x -> if x = scancode then value := i)
                keyboard_map
          done;
          set_register x 0x0
      | 0x15 ->
          (* LD DT, Vx *)
          delay_timer := rx
      | 0x18 ->
          (* LD ST, Vx *)
          sound_timer := rx;
          Beep.beep_play true
      | 0x1E ->
          (* ADD I, Vx *)
          register_I := !register_I + rx
      | 0x29 ->
          (* LD F, Vx *)
          register_I := rx * 5
      | 0x33 ->
          (* LD B, Vx *)
          Array.set memory (!register_I + 0) @@ (rx / 100 mod 10);
          Array.set memory (!register_I + 1) @@ (rx / 10 mod 10);
          Array.set memory (!register_I + 2) @@ (rx / 1 mod 10)
      | 0x55 ->
          (* LD [I], Vx *)
          for r = 0 to x do
            Array.set memory (r + !register_I) @@ read_register r
          done
      | 0x65 ->
          (* LD Vx, [I] *)
          for r = 0 to x do
            let v = Array.get memory (r + !register_I) in
            set_register r v
          done
      | _ -> invalid_instruction op);
  ]

(* Dispatch op to the right function *)
let exec_op op =
  (* Printf.printf "Executing %.4X\n" op; *)
  let kind = shift_and_mask op 3 0xF in
  List.nth op_table kind op

(* Load rom from a file into memory. If path is empty, load default program *)
let load_rom path =
  if path = "" then
    let default =
      [
        0x61;
        0x11;
        0x60;
        0x00;
        0x6A;
        0x04;
        0x6B;
        0x02;
        0x00;
        0xE0;
        0xFA;
        0x29;
        0xD0;
        0x15;
        0xFB;
        0x29;
        0x8C;
        0x00;
        0x7C;
        0x05;
        0xDC;
        0x15;
        0x70;
        0x01;
        0x12;
        0x08;
      ]
    in
    List.iteri (fun i el -> Array.set memory (0x200 + i) el) default
  else
    let file = open_in_bin path in
    let store = Bytes.create 0x1000 in
    let read = input file store 0 0x1000 in
    let store = Bytes.sub store 0 read in
    Bytes.iteri
      (fun i el -> Array.set memory (0x200 + i) (int_of_char el))
      store;
    Printf.printf "Loaded %d bytes into the RAM\n" read

let () =
  Beep.beep_init ();
  let time_acc = ref 0. in
  ignore (Sdl.init Sdl.Init.video);
  let window =
    match Sdl.create_window ~w:width ~h:height "Puce8" Sdl.Window.opengl with
    | Error _ -> assert false
    | Ok w -> w
  in
  let r =
    match Sdl.create_renderer window with Error _ -> assert false | Ok r -> r
  in
  renderer := Some r;
  clear_screen ();

  (* Load ROM from command-line argument path *)
  if Array.length Sys.argv > 1 then load_rom Sys.argv.(1) else load_rom "";

  (* Load sprites *)
  List.iteri (Array.set memory) sprites;

  (* Place PC at the beginning of the ROM *)
  program_counter := 0x200;

  (* MAIN LOOP *)
  let continue = ref true in
  while !continue do
    (* Event management *)
    let event = Sdl.Event.create () in
    while Sdl.poll_event (Some event) do
      let event_type = Sdl.Event.get event Sdl.Event.typ in
      if event_type = Sdl.Event.key_down then
        match
          Sdl.Scancode.enum @@ Sdl.Event.get event Sdl.Event.keyboard_scancode
        with
        | `Return ->
            (* RESET *)
            program_counter := 0x200;
            clear_screen ();
            for x = 0 to 63 do
              Array.fill screen.(x) 0 31 false
            done
        | `Escape -> continue := false (* HALT *)
        | _ -> ()
    done;
    (* dump_registers (); *)
    (* Debug *)
    Sdl.delay @@ Int32.of_int (1000 / int_of_float exec_fps);

    (* Get next instruction *)
    let op1 = Array.get memory !program_counter in
    let op2 = Array.get memory (!program_counter + 1) in
    let op = (op1 lsl 8) lor op2 in
    program_counter := !program_counter + 2;

    (* Execute it *)
    exec_op op;

    (* Update scren *)
    ignore (Sdl.render_present r);

    (* Update timers *)
    time_acc := !time_acc +. (1. /. exec_fps);
    if !time_acc > 1. /. audio_fps then (
      if !delay_timer > 0 then delay_timer := !delay_timer - 1;
      if !sound_timer > 0 then (
        sound_timer := !sound_timer - 1;
        if !sound_timer = 0 then Beep.beep_play false);
      time_acc := 0.)
  done;
  Sdl.destroy_renderer r;
  Sdl.destroy_window window;
  Sdl.quit ();
  exit 0
