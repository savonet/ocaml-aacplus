(*
 * Copyright 2003-2010 Savonet team
 *
 * This file is part of Ocaml-aacplus.
 *
 * Ocaml-aacplus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Ocaml-aacplus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ocaml-aacplus; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *)

 (** AAC+ encoding module for OCaml *)

(** {3 Usage} *)

(** Typical use of the encoding modules is:
  * 
  * {v  (* Define the encoding parameters: *)
  * let channels = (..) in
  * let samplerate = (..) in
  * (* Bitrate is in bps (e.g. 64000) *)
  * let bitrate = (..) in 
  * (* Create an encoder *)
  * let enc = Aacplus.create ~channels ~samplerate ~bitrate () in
  * (* Get the frame size *)
  * let samples = Aacplus.frame_size enc in
  * (* Encode some data *)
  * let data = (..some float array array value..) in
  * let ret = Aacplus.encode enc data in
  * (..do something with encoded data..)
  * (..repeat encoding process..) v}
  * 
  * Remarks:
  * - Documentation about valid and invalid configuration
  *   parameters is currently not available.
  * - See documentations for [encode] function about data
  *   submited for encoding. *)

(** {3 Types} *)

(** Type of an encoder *) 
type t

(** {3 Exceptions} *)

(** Raised when submiting invalid data for 
  * encoding *)
exception Invalid_data

(** Raised when a requesting an invalid 
  * encoding configuration. *)
exception Invalid_config

(** {3 Functions} *)

(** Create an encoder 
  *
  * Raises [Invalid_config] if encoding parameters
  * cannot be used by the library. *)
val create : channels:int -> samplerate:int -> bitrate:int -> unit -> t

(** Return the number of samples required for 
  * each channel of data submited for encoding *)
val frame_size : t -> int

(** Encode some audio data. 
  * 
  * Raises [Invalid_data] if submited data
  * does not have the number of channels given
  * when initiating the encoder or does not have
  * the number of samples returned by [frame_size]
  * in each channel. *)
val encode : t -> float array array -> string

