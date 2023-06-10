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
#include "interpreter/lang/slash_value.h"
#include "interpreter/lexer.h"
#ifndef SAC_TYPEDEF
#define SAC_TYPEDEF
#endif
#include "sac/sac.h"


/* types */
typedef enum {
    EXPR_UNARY = 0,
    EXPR_BINARY,
    EXPR_LITERAL,
    EXPR_INTERPOLATION,
    EXPR_ARG,
    EXPR_ENUM_COUNT
} ExprType;

typedef enum {
    STMT_EXPRESSION = 0,
    STMT_VAR,
    STMT_LOOP,
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
    SlashValue value;
} LiteralExpr;

typedef struct {
    ExprType type;
    Token *var_name;
} InterpolationExpr;

typedef struct arg_expr_t ArgExpr;
struct arg_expr_t {
    ExprType type;
    Expr *this;
    ArgExpr *next;
};


/* statements */
typedef struct {
    StmtType type;
    Expr *expression;
} ExpressionStmt;

typedef struct {
    StmtType type;
    Expr *condition;
    Stmt *body;
} LoopStmt;

typedef struct {
    StmtType type;
    Token *name;
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
    Token *cmd_name;
    ArgExpr *args_ll; // NULL terminated linked list
} CmdStmt;

typedef struct {
    StmtType type;
    Token *name;
    Token *assignment_op;
    Expr *value;
} AssignStmt;

typedef struct {
    StmtType type;
    ArenaLL *statements;
} BlockStmt;


/* functions */
Expr *expr_alloc(Arena *ast_arena, ExprType type);
Stmt *stmt_alloc(Arena *ast_arena, StmtType type);

void ast_print(struct arraylist_t *ast_heads);

void ast_arena_init(Arena *ast_arena);
void ast_arena_release(Arena *ast_arena);


#endif /* AST_H */
