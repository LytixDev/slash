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

#include "arena_ll.h"
#include "interpreter/lexer.h"
#include "interpreter/types/slash_value.h"
#include "sac/sac.h"


/* types */
typedef enum {
    EXPR_UNARY = 0,
    EXPR_BINARY,
    EXPR_LITERAL,
    EXPR_INTERPOLATION,
    EXPR_SUBSHELL,
    EXPR_LIST,
    EXPR_ENUM_COUNT
} ExprType;

typedef enum {
    STMT_EXPRESSION = 0,
    STMT_VAR,
    STMT_LOOP,
    STMT_ITER_LOOP,
    STMT_IF,
    STMT_CMD,
    STMT_ASSIGN,
    STMT_BLOCK,
    STMT_ENUM_COUNT
} StmtType;

extern char *expr_type_str_map[EXPR_ENUM_COUNT];
extern char *stmt_type_str_map[STMT_ENUM_COUNT];


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
    TokenType operator_;
    Expr *right;
} UnaryExpr;

typedef struct {
    ExprType type;
    Expr *left;
    TokenType operator_;
    Expr *right;
} BinaryExpr;

typedef struct {
    ExprType type;
    SlashValue value;
} LiteralExpr;

typedef struct {
    ExprType type;
    StrView var_name;
    int index; // optional
} InterpolationExpr;

typedef struct {
    ExprType type;
    Stmt *stmt;
} SubshellExpr;

typedef struct {
    ExprType type;
    ArenaLL *exprs;
} ListExpr;


/* statements */
typedef struct {
    StmtType type;
    Expr *expression;
} ExpressionStmt;

typedef struct {
    StmtType type;
    ArenaLL *statements;
} BlockStmt;

typedef struct {
    StmtType type;
    Expr *condition;
    BlockStmt *body_block;
} LoopStmt;

typedef struct {
    StmtType type;
    StrView var_name;
    Expr *underlying_iterable;
    BlockStmt *body_block;
} IterLoopStmt;

typedef struct {
    StmtType type;
    StrView name;
    Expr *initializer;
} VarStmt;

typedef struct {
    StmtType type;
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch; // optional
} IfStmt;

typedef struct {
    StmtType type;
    StrView cmd_name;
    ArenaLL *arg_exprs;
} CmdStmt;

typedef struct {
    StmtType type;
    StrView name;
    TokenType assignment_op;
    Expr *value;
} AssignStmt;


/* functions */
Expr *expr_alloc(Arena *ast_arena, ExprType type);
Stmt *stmt_alloc(Arena *ast_arena, StmtType type);

void ast_print(ArrayList *ast_heads);

void ast_arena_init(Arena *ast_arena);
void ast_arena_release(Arena *ast_arena);


#endif /* AST_H */
