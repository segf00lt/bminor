#ifndef LEXER_H
#define LEXER_H

#define LEXER_KEYWORD_TOKENS \
/* upper,          lower,            source_string,        precedence   */ \
X(IF,              if,               "if",                 0             ) \
X(WHILE,           while,            "while",              0             ) \
X(FOR,             for,              "for",                0             ) \
X(ELSE,            else,             "else",               0             ) \
X(BREAK,           break,            "break",              0             ) \
X(CONTINUE,        continue,         "continue",           0             ) \


#define LEXER_TOKENS \
/* upper,          lower,            source_string,        precedence   */ \
X(EOL,             eol,              "",                   0             ) \
X(NUM,             num,              "",                   0             ) \
X(PLUS,            plus,             "",                   0             ) \
X(MINUS,           minus,            "",                   0             ) \
X(TIMES,           times,            "",                   0             ) \
X(DIV,             div,              "",                   0             ) \
X(ERROR,           error,            "",                   0             ) \
LEXER_KEYWORD_TOKENS \


typedef enum Token {
  TOKEN_MIN_VALUE = -1,
  TOKEN_END_ASCII_RANGE = 255,
  #define X(upper, lower, source_string, precedence) TOKEN_##upper,
  LEXER_TOKENS
  #undef X
  TOKEN_MAX_VALUE,
} Token;

typedef struct Source_location {
  Str8 src_path;
  int line;
  int col;
  Str8 line_text;
} Source_location;


typedef struct Lexer {
  Token token;
  Str8  src_path;
  Str8  src;
  s64   pos;
  Str8  token_text;

  Source_location location;
  Source_location location_next;
} Lexer;



#endif
