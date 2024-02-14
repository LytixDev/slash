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

#include "interpreter/lexer.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "sac/sac.h"


/* types */
typedef enum {
    EXPR_UNARY = 0,
    EXPR_BINARY,
    EXPR_LITERAL,
    EXPR_ACCESS,
    EXPR_SUBSCRIPT,
    EXPR_SUBSHELL,
    EXPR_STR,
    EXPR_LIST,
    EXPR_FUNCTION,
    EXPR_MAP,
    EXPR_METHOD,
    EXPR_SEQUENCE,
    EXPR_GROUPING,
    EXPR_CAST,
    EXPR_CALL,
    EXPR_ENUM_COUNT
} ExprType;

typedef enum {
    STMT_EXPRESSION = 0,
    STMT_VAR,
    STMT_SEQ_VAR,
    STMT_LOOP,
    STMT_ITER_LOOP,
    STMT_IF,
    STMT_CMD,
    STMT_ASSIGN,
    STMT_BLOCK,
    STMT_PIPELINE,
    STMT_ASSERT,
    STMT_BINARY,
    STMT_ABRUPT_CONTROL_FLOW,
    STMT_ENUM_COUNT
} StmtType;

extern char *expr_type_str_map[EXPR_ENUM_COUNT];
extern char *stmt_type_str_map[STMT_ENUM_COUNT];


/*
 * evil trick to get some sort of polymorphism (I concede, my brain has been corrupted by OOP)
 */
typedef struct expr_t {
    ExprType type;
} Expr;

typedef struct {
    StmtType type;
} Stmt;

typedef struct {
    ExprType type;
    ArenaLL seq; // pointers to Exprs
} SequenceExpr;

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
} AccessExpr;

typedef struct {
    ExprType type;
    Expr *expr; // the underlying expr we are subscripting
    Expr *access_value; // a[x], where x in this case is the 'access_value'
} SubscriptExpr;

typedef struct {
    ExprType type;
    Stmt *stmt;
} SubshellExpr;

typedef struct {
    ExprType type;
    StrView view;
} StrExpr;

typedef struct {
    ExprType type;
    SequenceExpr *exprs; // will be NULL for the empty list
} ListExpr;

typedef struct {
    StmtType type;
    ArenaLL params; // list of parameter names as pointers to StrView's
    BlockStmt *body;
} FunctionExpr;


// NOTE: not an expression
typedef struct {
    Expr *key;
    Expr *value;
} KeyValuePair;

typedef struct {
    ExprType type;
    ArenaLL *key_value_pairs;
} MapExpr;

typedef struct {
    ExprType type;
    Expr *obj;
    StrView method_name;
    SequenceExpr *args;
} MethodExpr;

typedef struct {
    ExprType type;
    Expr *expr;
} GroupingExpr;

typedef struct {
    ExprType type;
    Expr *expr;
    StrView type_name;
} CastExpr;

typedef struct {
    ExprType type;
    Expr *callee;
    SequenceExpr *args;
} CallExpr;


/* statements */
typedef struct {
    StmtType type;
    Expr *expression;
} ExpressionStmt;

typedef struct slash_block_stmt_t {
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
    ArenaLL names; // list of variable names as pointers to StrView's
    Expr *initializer;
} SeqVarStmt;

typedef struct {
    StmtType type;
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch; // optional
} IfStmt;

typedef struct {
    StmtType type;
    StrView cmd_name;
    ArenaLL *arg_exprs; // list of exprs
} CmdStmt;

typedef struct {
    StmtType type;
    Expr *var; // AccessExpr or ItemAccessExpr
    TokenType assignment_op;
    Expr *value;
} AssignStmt;

typedef struct {
    StmtType type;
    CmdStmt *left;
    Stmt *right;
} PipelineStmt;

typedef struct {
    StmtType type;
    Expr *expr;
} AssertStmt;

typedef struct {
    StmtType type;
    Stmt *left;
    TokenType operator_;
    union { // if operator_ is t_greater then union is expr
	Stmt *right_stmt;
	Expr *right_expr;
    };
} BinaryStmt;

typedef struct {
    StmtType type;
    TokenType ctrlf_type; // t_break, t_continue or t_return
    Expr *return_expr;
} AbruptControlFlowStmt;


/* functions */
Expr *expr_alloc(Arena *ast_arena, ExprType type);
Stmt *stmt_alloc(Arena *ast_arena, StmtType type);
Stmt *stmt_copy(Arena *arena, Stmt *to_copy);
Expr *expr_copy(Arena *arena, Expr *to_copy);

void ast_print(ArrayList *ast_heads);

void ast_arena_init(Arena *ast_arena);
void ast_arena_release(Arena *ast_arena);


#endif /* AST_H */
