/*
 *  Copyright (C) 2023 Nicolai Brand (https://lytix.dev)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AST_H
#define AST_H

#include "lexer.h"
#ifndef SAC_TYPEDEF
#define SAC_TYPEDEF
#endif
#include "sac/sac.h"


/* types */
typedef enum { EXPR_UNARY, EXPR_BINARY, EXPR_LITERAL, EXPR_ENUM_COUNT } ExprType;

typedef enum { STMT_EXPRESSION, STMT_VAR, STMT_ENUM_COUNT } StmtType;


/*
 * evil trick to get some sort of polymorphism (I concede, my brain has been corrupted by OOP)
 */
typedef struct {
    ExprType type;
} Expr;

typedef struct {
    StmtType type;
} Stmt;

/* expressions */
typedef struct {
    ExprType type;
    Token *operator_;
    Expr *right;
} UnaryExpr;

typedef struct {
    ExprType type;
    Expr *left;
    Token *operator_;
    Expr *right;
} BinaryExpr;

typedef struct {
    ExprType type;
    TokenType value_type; // dt_str, dt_num, dt_bool ...
    void *value;
} LiteralExpr;


/* statements */
typedef struct {
    StmtType type;
    Expr *expression;
} ExpressionStmt;

typedef struct {
    StmtType type;
    Token *name;
    Expr *initializer;
} VarStmt;


/* functions */
Expr *expr_alloc(Arena *ast_arena, ExprType type);
Stmt *stmt_alloc(Arena *ast_arena, StmtType type);

void ast_print(struct darr_t *ast_heads);

/* do I really need a memory arena? Can all expr and stmts just be stack allocated? */
void ast_arena_init(Arena *ast_arena);
void ast_arena_release(Arena *ast_arena);


#endif /* AST_H */
