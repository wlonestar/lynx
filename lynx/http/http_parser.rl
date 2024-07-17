/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lynx/http/http_parser.h"

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cctype>
#include <cstring>

#define LEN(AT, FPC) (FPC - buffer - AT)
#define MARK(M, FPC) (M = (FPC) - buffer)
#define PTR_TO(F) (buffer + F)

using namespace lynx;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunused-const-variable"

/** Machine **/

%%{
  
  machine http_parser;

  action mark { MARK(mark_, fpc); }

  action start_field { MARK(field_start_, fpc); }
  action write_field { 
    field_len_ = LEN(field_start_, fpc);
  }

  action start_value { MARK(mark_, fpc); }

  action write_value {
    if(http_field_ != NULL) {
      http_field_(data_, PTR_TO(field_start_), field_len_, PTR_TO(mark_), LEN(mark_, fpc));
    }
  }

  action request_method { 
    if(request_method_ != NULL) 
      request_method_(data_, PTR_TO(mark_), LEN(mark_, fpc));
  }

  action request_uri { 
    if(request_uri_ != NULL)
      request_uri_(data_, PTR_TO(mark_), LEN(mark_, fpc));
  }

  action fragment {
    if(fragment_ != NULL)
      fragment_(data_, PTR_TO(mark_), LEN(mark_, fpc));
  }

  action start_query {MARK(query_start_, fpc); }
  action query_string { 
    if(query_string_ != NULL)
      query_string_(data_, PTR_TO(query_start_), LEN(query_start_, fpc));
  }

  action http_version {	
    if(http_version_ != NULL)
      http_version_(data_, PTR_TO(mark_), LEN(mark_, fpc));
  }

  action request_path {
    if(request_path_ != NULL)
      request_path_(data_, PTR_TO(mark_), LEN(mark_,fpc));
  }

  action done {
      if(xml_sent_ || json_sent_) {
        body_start_ = PTR_TO(mark_) - buffer;
        // +1 includes the \0
        content_len_ = fpc - buffer - body_start_ + 1;
      } else {
        body_start_ = fpc - buffer + 1;

        if(header_done_ != NULL) {
          header_done_(data_, fpc + 1, pe - fpc - 1);
        }
      }
    fbreak;
  }

  action xml {
      xml_sent_ = 1;
  }

  action json {
      json_sent_ = 1;
  }


#### HTTP PROTOCOL GRAMMAR
  CRLF = ( "\r\n" | "\n" ) ;

  # URI description as per RFC 3986.

  more_delims   = ( "{" | "}" | "^" ) when { uri_relaxed_ } ;
  sub_delims    = ( "!" | "$" | "&" | "'" | "(" | ")" | "*"
                  | "+" | "," | ";" | "=" | more_delims ) ;
  gen_delims    = ( ":" | "/" | "?" | "#" | "[" | "]" | "@" ) ;
  reserved      = ( gen_delims | sub_delims ) ;
  unreserved    = ( alpha | digit | "-" | "." | "_" | "~" ) ;

  pct_encoded   = ( "%" xdigit xdigit ) ;

# pchar         = ( unreserved | pct_encoded | sub_delims | ":" | "@" ) ;
# add (any -- ascii) support chinese
  pchar         = ( (any -- ascii) | unreserved | pct_encoded | sub_delims | ":" | "@" ) ;

  fragment      = ( ( pchar | "/" | "?" )* ) >mark %fragment ;

  query         = ( ( pchar | "/" | "?" )* ) %query_string ;

  # non_zero_length segment without any colon ":" ) ;
  segment_nz_nc = ( ( unreserved | pct_encoded | sub_delims | "@" )+ ) ;
  segment_nz    = ( pchar+ ) ;
  segment       = ( pchar* ) ;

  path_empty    = ( pchar{0} ) ;
  path_rootless = ( segment_nz ( "/" segment )* ) ;
  path_noscheme = ( segment_nz_nc ( "/" segment )* ) ;
  path_absolute = ( "/" ( segment_nz ( "/" segment )* )? ) ;
  path_abempty  = ( ( "/" segment )* ) ;

  path          = ( path_abempty    # begins with "/" or is empty
                  | path_absolute   # begins with "/" but not "//"
                  | path_noscheme   # begins with a non-colon segment
                  | path_rootless   # begins with a segment
                  | path_empty      # zero characters
                  ) ;

  reg_name      = ( unreserved | pct_encoded | sub_delims )* ;

  dec_octet     = ( digit                 # 0-9
                  | ("1"-"9") digit         # 10-99
                  | "1" digit{2}          # 100-199
                  | "2" ("0"-"4") digit # 200-249
                  | "25" ("0"-"5")      # 250-255
                  ) ;

  IPv4address   = ( dec_octet "." dec_octet "." dec_octet "." dec_octet ) ;
  h16           = ( xdigit{1,4} ) ;
  ls32          = ( ( h16 ":" h16 ) | IPv4address ) ;

  IPv6address   = (                               6( h16 ":" ) ls32
                  |                          "::" 5( h16 ":" ) ls32
                  | (                 h16 )? "::" 4( h16 ":" ) ls32
                  | ( ( h16 ":" ){1,} h16 )? "::" 3( h16 ":" ) ls32
                  | ( ( h16 ":" ){2,} h16 )? "::" 2( h16 ":" ) ls32
                  | ( ( h16 ":" ){3,} h16 )? "::"    h16 ":"   ls32
                  | ( ( h16 ":" ){4,} h16 )? "::"              ls32
                  | ( ( h16 ":" ){5,} h16 )? "::"              h16
                  | ( ( h16 ":" ){6,} h16 )? "::"
                  ) ;

  IPvFuture     = ( "v" xdigit+ "." ( unreserved | sub_delims | ":" )+ ) ;

  IP_literal    = ( "[" ( IPv6address | IPvFuture  ) "]" ) ;

  port          = ( digit* ) ;
  host          = ( IP_literal | IPv4address | reg_name ) ;
  userinfo      = ( ( unreserved | pct_encoded | sub_delims | ":" )* ) ;
  authority     = ( ( userinfo "@" )? host ( ":" port )? ) ;

  scheme        = ( alpha ( alpha | digit | "+" | "-" | "." )* ) ;

  relative_part = ( "//" authority path_abempty
                  | path_absolute
                  | path_noscheme
                  | path_empty
                  ) ;


  hier_part     = ( "//" authority path_abempty
                  | path_absolute
                  | path_rootless
                  | path_empty
                  ) ;

  absolute_URI  = ( scheme ":" hier_part ( "?" query )? ) ;

  relative_ref  = ( (relative_part %request_path ( "?" %start_query query )?) >mark %request_uri ( "#" fragment )? ) ;
  URI           = ( scheme ":" (hier_part  %request_path ( "?" %start_query query )?) >mark %request_uri ( "#" fragment )? ) ;

  URI_reference = ( URI | relative_ref ) ;

# HTTP header parsing
  Method = ( upper | digit ){1,20} >mark %request_method;

  http_number = ( "1." ("0" | "1") ) ;
  HTTP_Version = ( "HTTP/" http_number ) >mark %http_version ;
  Request_Line = ( Method " " URI_reference " " HTTP_Version CRLF ) ;

  HTTP_CTL = (0 - 31) | 127 ;
  HTTP_separator = ( "(" | ")" | "<" | ">" | "@"
                   | "," | ";" | ":" | "\\" | "\""
                   | "/" | "[" | "]" | "?" | "="
                   | "{" | "}" | " " | "\t"
                   ) ;

  lws = CRLF? (" " | "\t")+ ;
  token = ascii -- ( HTTP_CTL | HTTP_separator ) ;
  content = ((any -- HTTP_CTL) | lws);

  field_name = ( token )+ >start_field %write_field;

  field_value = content* >start_value %write_value;

  message_header = field_name ":" lws* field_value :> CRLF;

  Request = Request_Line ( message_header )* ( CRLF );

  SocketJSONStart = ("@" relative_part);
  SocketJSONData = "{" any* "}" :>> "\0";

  SocketXMLData = ("<" [a-z0-9A-Z\-.]+) >mark %request_path ("/" | space | ">") any* ">" :>> "\0";

  SocketJSON = SocketJSONStart >mark %request_path " " SocketJSONData >mark @json;
  SocketXML = SocketXMLData @xml;

  SocketRequest = (SocketXML | SocketJSON);

main := (Request | SocketRequest) @done;

}%%

/** Data **/
%% write data;

int HttpParser::init() {
  int cs = 0;
  %% write init;
  cs_ = cs;
  body_start_ = 0;
  content_len_ = 0;
  mark_ = 0;
  nread_ = 0;
  field_len_ = 0;
  field_start_ = 0;
  xml_sent_ = 0;
  json_sent_ = 0;
  return(1);
}

int HttpParser::finish() {
  if (hasError() ) {
    return -1;
  } else if (isFinished() ) {
    return 1;
  } else {
    return 0;
  }
}

size_t HttpParser::execute(const char *buffer, size_t len, size_t off) {
  if(len == 0) return 0;
  nread_ = 0;
  mark_ = 0;
  field_len_ = 0;
  field_start_ = 0;
 
  const char *p, *pe;
  int cs = cs_;

  assert(off <= len && "offset past end of buffer");

  p = buffer+off;
  pe = buffer+len;

  assert(pe - p == (int)len - (int)off && "pointers aren't same distance");

  %% write exec;

  assert(p <= pe && "Buffer overflow after parsing.");

  if (!hasError()) {
      cs_ = cs;
  }

  nread_ += p - (buffer + off);

  assert(nread_ <= len && "nread longer than length");
  assert(body_start_ <= len && "body starts after buffer end");
  assert(mark_ < len && "mark is after buffer end");
  assert(field_len_ <= len && "field has length longer than whole buffer");
  assert(field_start_ < len && "field starts after buffer end");

  return(nread_);
}

int HttpParser::hasError() {
  return cs_ == http_parser_error;
}

bool HttpParser::isFinished() {
  return cs_ >= http_parser_first_final;
}

#pragma clang diagnostic pop
