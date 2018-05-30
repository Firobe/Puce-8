open Tsdl
open Result

let pi = 3.14159265359
let beep_device = ref None
let beep_callback_ref = ref None
let beep_playing = ref false
let beep_last = ref false
let beep_pos = ref 0
let beep_sampling = ref 0
let beep_frequency = 440.
let beep_volume = 6000.
let beep_callback buffer =
    for i = 0 to Bigarray.Array1.dim buffer - 1 do
        if !beep_playing || !beep_last then (
            let t = float_of_int !beep_pos in
            let s = float_of_int !beep_sampling in
            let v = beep_frequency *. 2. *. pi *. t /. s in
            buffer.{i} <- int_of_float @@ beep_volume *. sin v ;
            beep_pos := !beep_pos + 1 ;
            if !beep_last && (buffer.{i} = 0 ||
                (i > 1 && (buffer.{i} > 0) != (buffer.{i-1} > 0))) then (
                buffer.{i} <- 0;
                beep_last := false ;
            )
        )
        else (
            buffer.{i} <- 0 ;
            beep_pos := 0
        )
    done

let beep_init () =
    match Sdl.init Sdl.Init.audio with
        | Error (`Msg e) ->
            Printf.printf "Error while initializing audio device %s" e;
            assert false
        | Ok () -> () ;
    beep_callback_ref :=
        Some (Sdl.audio_callback Bigarray.Int16_signed beep_callback) ;
    let audio_spec = {
        Sdl.as_freq = 44100;
        as_format = Sdl.Audio.s16;
        as_channels = 1;
        as_silence = 0;
        as_samples = 4096;
        as_size = Int32.zero;
        as_callback = !beep_callback_ref
    } in
    let (dev, have) = match Sdl.open_audio_device None false audio_spec
        Sdl.Audio.allow_frequency_change with
        | Error (`Msg e) ->
            Printf.printf "Error while opening audio device: %s\n" e;
            assert false
    | Ok h -> h
    in
    beep_sampling := have.as_freq ;
    beep_device := Some dev ;
    Sdl.pause_audio_device dev false


let beep_play b =
    beep_playing := b ;
    if not b then beep_last := true
