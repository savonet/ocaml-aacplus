(*
 * Copyright 2003-2009 Savonet team
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

 (** OCaml bindings for the libaacplus. *)

type t

exception Invalid_data
exception Invalid_config

val create : channels:int -> samplerate:int -> bitrate:int -> unit -> t

val frame_size : t -> int

val encode : t -> float array array -> string

