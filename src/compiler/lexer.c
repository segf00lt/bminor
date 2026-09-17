#ifndef LEXER_C
#define LEXER_C


global read_only char lexer_token_reserved_chars[] = "!\"#$%&\'()*+,-./:;<=>?@[\\]^`{|}~";

global read_only Token lexer_keyword_tokens[] = {
  #define X(upper, lower, source_string, precedence) TOKEN_##upper,
  LEXER_KEYWORD_TOKENS
  #undef X
};

global read_only Str8 lexer_keyword_token_strings[] = {
  #define X(upper, lower, source_string, precedence) str8_lit(source_string),
  LEXER_KEYWORD_TOKENS
  #undef X
};

global read_only Str8 lexer_token_debug_strings[TOKEN_MAX_VALUE] = {
  #define X(upper, lower, source_string, precedence) [TOKEN_##upper] = str8_lit("TOKEN_"#upper),
  LEXER_TOKENS
  #undef X
};

STATIC_ASSERT(ARRLEN(lexer_keyword_token_strings) == ARRLEN(lexer_keyword_tokens), keyword_token_count);


internal s64
func str8_pos_of_substr_in_str(Str8 substr, Str8 str) {
  s64 pos = (s64)(substr.s - str.s);

  if(pos < 0 || pos >= str.len) {
    pos = -1;
  }

  return pos;
}

#define is_word_char(c) (!!( is_alpha(c) || is_decimal(c) || ((c) == '_') ))


internal void
func init_lexer(Lexer *l, Str8 src_path, Str8 src) {
  *l = (Lexer){0};
  l->src_path = src_path;
  l->token = TOKEN_MIN_VALUE;
  l->src = src;
  l->location.src_path = src_path;
  l->location.line = l->location.col = 1;
  l->location.line_text = str8_slice(src, 0, str8_find_char(src, '\n'));
  l->location_next = l->location;
}

internal Token
func lex(Lexer *l) {

  if(l->token == 0) {
    return 0;
  }

  l->token_text = (Str8){0};

  // NOTE jfd: skip whitespace and comments (copied from jcc source)
  b8 found_newline = false;
  while(true) {
    while(is_space(l->src.s[l->pos])) { /* whitespace */
      if(l->src.s[l->pos] != '\n') {
        ++l->location_next.col;
      } else {
        l->location_next.col = 1;
        ++l->location_next.line;
        found_newline = true;
      }

      ++l->pos;
    }

    /* single-line comments */
    if((l->src.s[l->pos + 0] == '/' && l->src.s[l->pos + 1] == '/')) {
      while(l->src.s[l->pos] != '\n')
      ++l->pos;
      ++l->pos;
      l->location_next.col = 1;
      ++l->location_next.line;
      found_newline = true;
    } else if(l->src.s[l->pos + 0] == '/' && l->src.s[l->pos + 1] == '*') { /* multi-line comments */
      while(!(l->src.s[l->pos + 0] == '*' && l->src.s[l->pos + 1] == '/')) {
        if(l->src.s[l->pos] != '\n') {
          ++l->location_next.col;
        } else {
          l->location_next.col = 1;
          ++l->location_next.line;
          found_newline = true;
        }

        ++l->pos;
      }
      l->location_next.col += 2;
      l->pos += 2;
    } else {
      break;
    }
  }

  if(l->src.len <= l->pos) {
    return 0;
  }

  if(found_newline) {
    s64 location_pos = l->pos;
    while(location_pos >= 0 && l->src.s[location_pos - 1] != '\n') location_pos--;
    location_pos = CLAMP_BOT(location_pos, 0);
    l->location_next.line_text = str8_slice(l->src, location_pos, -1);
    l->location_next.line_text.len = str8_find_char(l->location_next.line_text, '\n');
  }
  l->location = l->location_next;

  Str8 cur_text = str8_slice(l->src, l->pos, -1);

  // keywords
  for(int i = 0; i < ARRLEN(lexer_keyword_tokens); i++) {
    Token keyword_token = lexer_keyword_tokens[i];
    Str8 cur_keyword = lexer_keyword_token_strings[i];

    if(cur_text.len < cur_keyword.len) {
      continue;
    }

    b32 starts_with_keyword = str8_starts_with(cur_text, cur_keyword);
    b32 ends_in_non_word_char = !is_word_char(cur_text.s[cur_keyword.len]);

    if(starts_with_keyword && ends_in_non_word_char) {
      l->token = keyword_token;
      l->token_text = cur_keyword;
      l->pos += cur_keyword.len;
      l->location_next.col += cur_keyword.len;
      return l->token;
    }

  }

  // literals
  {
    if(is_decimal(cur_text.s[0])) {
      Str8 whole_part = {0};
      Str8 numeric_literal_text = {0};

      int i = 0;
      while(i < cur_text.len && is_decimal(cur_text.s[i])) {
        i++;
      }

      whole_part = str8_slice(cur_text, 0, i);

      if(cur_text.s[i] == '.') {
        i++;
        while(i < cur_text.len && is_decimal(cur_text.s[i])) {
          i++;
        }
        numeric_literal_text = str8_slice(cur_text, 0, i);

        // TODO add floats
        l->token = TOKEN_NUM;
        l->token_text = numeric_literal_text;
        l->pos += numeric_literal_text.len;
        l->location_next.col += numeric_literal_text.len;

      } else {
        numeric_literal_text = whole_part;

        l->token = TOKEN_NUM;
        l->token_text = numeric_literal_text;
        l->pos += numeric_literal_text.len;
        l->location_next.col += numeric_literal_text.len;

      }

      return l->token;
    }
  }

  // reserved chars ('*', '+', ',', '/', etc)
  for(int i = 0; i < (int)STRLEN(lexer_token_reserved_chars); i++) {
    if(cur_text.s[0] == (u8)lexer_token_reserved_chars[i]) {
      l->token = cur_text.s[0];
      l->token_text = cur_text;
      l->token_text.len = 1;
      l->pos++;
      l->location_next.col++;
      return l->token;
    }
  }

  // identifiers

  l->token = TOKEN_ERROR;
  return l->token;
}


#if 1

int main(int argc, char **argv) {
  Arena *a = arena_create(MB(1));

  Str8 src_path = str8_lit("tests/lexer_test.c");
  Str8 src = platform_read_entire_file(a, (char*)src_path.s);

  Lexer lexer;
  init_lexer(&lexer, src_path, src);


  for(Token token = lex(&lexer); token != 0; token = lex(&lexer)) {
    arena_scope(a) {
      printf("token: %d %s, token_text: %s, location: line %d col %d \"%s\"\n",
        token,
        cstr_from_str8(a, lexer_token_debug_strings[token]),
        cstr_from_str8(a, lexer.token_text),
        lexer.location.line,
        lexer.location.col,
        cstr_from_str8(a, lexer.location.line_text)
      );
    }
  }


  return 0;
}

#endif



#endif
