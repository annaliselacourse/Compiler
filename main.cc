//Annalise LaCourse

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <sstream>
#include "execute.h"
#include "lexer.h"
#include <vector>

using namespace std;

Lexer lexer;  

Node * head = NULL;
Node * previous = NULL;
Node * current = NULL;

struct variable {
    string name;
    int address;
};

//list of variables
vector<variable> variables;

void parse_body();
void parse_condition();
void parse_assign_stmt();

//gets the location of a ID or NUM (VARIABLE or CONST)
int location(string name) {
    int address = -1;
    for (int i = 0; i < variables.size(); i++) {
        if (variables[i].name == name) {
            address = variables[i].address;
        }
    }

    //if the name is not i the variables list then it is a const
    //create a new mem location for the const
    if (address == -1) {
        variable c;
        c.name = name;
        c.address = next_available;
        address = next_available;
        int i = stoi(name);
        mem[next_available] = i;
        variables.push_back(c);
        next_available++;
    }

    return address;
} 

void parse_inputs() {
    Token t = lexer.ParseToken();//NUM
    int i = stoi(t.lexeme);
    inputs.push_back(i);

    t = lexer.peek(1);
    if (t.token_type == NUM) {
        parse_inputs();
    }
}

void parse_case_list(int switch_param, Node * noop_end) {
    //if statement
    lexer.ParseToken();//CASE
    Token t = lexer.ParseToken();//NUM
    int index_of_case_num = location(t.lexeme);
    lexer.ParseToken();//COLON
    struct nNode * case1 = new Node;
    case1->type = CJMP;
    case1->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    case1->cjmp_inst.opernd1_index = switch_param;
    case1->cjmp_inst.opernd2_index = index_of_case_num;
    if (head != NULL) {
        previous = current;
        current = case1;
        previous->next = current;
    } else {
        head = case1;
        current = case1;
    }

    //body of if statement (a NOTEQUAL case_num)
    //jmp to next case
    struct Node * jmp_next_case = new Node;
    jmp_next_case->type = JMP;
    previous = current;
    current = jmp_next_case;
    previous->next = current;

    //after if statement
    //create noop to jump to from case1
    struct Node * noop_case1 = new Node;
    noop_case1->type = NOOP;
    case1->cjmp_inst.target = noop_case1;
    previous = current;
    current = noop_case1;
    previous->next = current;
    //body of case1
    parse_body();//body
    //jmp to end of switch stmt
    struct Node * jmp_end = new Node;
    jmp_end->type = JMP;
    jmp_end->jmp_inst.target = noop_end;
    previous = current;
    current = jmp_end;
    previous->next = current;

    //noop to get to next case
    struct Node * noop_next = new Node;
    noop_next->type = NOOP;
    jmp_next_case->jmp_inst.target = noop_next;
    previous = current;
    current = noop_next;
    previous->next = current;

    t = lexer.peek(1);
    if (t.token_type == CASE) {
        parse_case_list(switch_param, noop_end);
    }
    
}

void parse_for_stmt() {
    lexer.ParseToken();//FOR
    lexer.ParseToken();//LPAREN

    parse_assign_stmt();

    //create cjmp instruction to jump to end of loop
    struct Node * cjmp = new Node;
    cjmp->type = CJMP;
    previous = current;
    current = cjmp;
    previous->next = current;
    
    parse_condition();

    lexer.ParseToken();//SEMICOLON

    parse_assign_stmt();//current will be equal to the assign node after this
    struct Node * i_increment = current;
    struct Node * noop = new Node;
    noop->type = NOOP;
    current = noop;
    previous->next = current;

    lexer.ParseToken();//RPAREN

    parse_body();

    //increment i
    previous = current;
    current = i_increment;
    previous->next = current;

    struct Node * jmp_for = new Node;
    jmp_for->type = JMP;
    jmp_for->jmp_inst.target = cjmp;
    previous = current;
    current = jmp_for;
    previous->next = current;

    struct Node * noop_end_for = new Node;
    noop_end_for->type = NOOP;
    cjmp->cjmp_inst.target = noop_end_for;
    previous = current;
    current = noop_end_for;
    previous->next = current;


}

void parse_switch_stmt() {
    lexer.ParseToken();//SWITCH
    Token t = lexer.ParseToken();//ID
    int switch_param = location(t.lexeme);
    lexer.ParseToken();//LBRACE

   //create new instruction node to be used as noop at the end of the switch statement
    struct Node * noop_switch = new Node;
    noop_switch->type = NOOP;
    parse_case_list(switch_param, noop_switch);//case_list
    t = lexer.peek(1);
    if (t.token_type == DEFAULT) {
        lexer.ParseToken();//DEFAULT
        lexer.ParseToken();//COLON
        parse_body();
    }

    lexer.ParseToken();//RBRACE

    previous = current;
    current = noop_switch;
    previous->next = current;
}

void parse_condition() {
    Token t = lexer.ParseToken();//primary
    current->cjmp_inst.opernd1_index = location(t.lexeme);

    t = lexer.ParseToken();//relop
    if (t.token_type == GREATER) {
        current->cjmp_inst.condition_op = CONDITION_GREATER;
    } else if (t.token_type == LESS) {
        current->cjmp_inst.condition_op = CONDITION_LESS;
    } else if (t.token_type == NOTEQUAL) {
        current->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    }

    t = lexer.ParseToken();//primary
    current->cjmp_inst.opernd2_index = location(t.lexeme);
}

void parse_if_stmt() {
    lexer.ParseToken();//IF
    //create new instruction node
    struct Node * cjmp = new Node;
    if (head != NULL) {
        previous = current;
        current = cjmp;
        previous->next = cjmp;
    } else {
        head = cjmp;
        current = cjmp;
    }

    current->type = CJMP;
    parse_condition();
    parse_body();

    //create noop instruction
    struct Node * noop = new Node;
    cjmp->cjmp_inst.target = noop;
    previous = current;
    current = noop;
    previous->next = current;

    current->type = NOOP;
}

void parse_while_stmt() {
    lexer.ParseToken();//WHILE

    //create cjmp instruction
    struct Node * cjmp = new Node;

    if (head != NULL) {
        previous = current;
        current = cjmp;
        previous->next = current;
    } else {
        head = cjmp;
        current = cjmp;
    }

    current->type = CJMP;
    parse_condition();
    parse_body();

    //create jmp instruction
    struct Node * jmp = new Node;
    previous = current;
    current = jmp;
    previous->next = current;

    current->type = JMP;
    current->jmp_inst.target = cjmp;

    //create noop instruction
    struct Node * noop = new Node;
    cjmp->cjmp_inst.target = noop;
    previous = current;
    current = noop;
    previous->next = current;

    current->type = NOOP;

}

void parse_input_stmt() {
    lexer.ParseToken();//INPUT
    //create new instruction node
    struct Node * node = new Node;
    if (head != NULL) {
        previous = current;
        current = node;
        previous->next = current;
    } else {
        head = node;
        current = node;
    }

    current->type = IN;
    Token t = lexer.ParseToken();//ID
    current->input_inst.var_index = location(t.lexeme);
    lexer.ParseToken();//SEMICOLON
}

void parse_output_stmt() {
    lexer.ParseToken();//OUTPUT
    //create new instruction node
    struct Node * node = new Node;
    if (head != NULL) {
        previous = current;
        current = node;
        previous->next = current;
    } else {
        head = node;
        current = node;
    }

    current->type = OUT;
    Token t = lexer.ParseToken();//ID
    current->output_inst.var_index = location(t.lexeme);
    lexer.ParseToken();//SEMICOLON
}

void parse_assign_stmt() {
    //create new instruction node
    struct Node * node = new Node;
    if (head != NULL) {
        previous = current;
        current = node;
        previous->next = current;
    } else {
        head = node;
        current = node;
    }

    current->type = ASSIGN;

    Token t = lexer.ParseToken();//ID
    //set the left hand side index to that variable's location
    current->assign_inst.left_hand_side_index = location(t.lexeme);

    lexer.ParseToken();//EQUAL
    t = lexer.ParseToken();//primary
    current->assign_inst.opernd1_index = location(t.lexeme);

    t = lexer.peek(1);
    if (t.token_type == SEMICOLON) {
        current->assign_inst.op = OPERATOR_NONE;

    } else if (t.token_type == PLUS) {
        current->assign_inst.op = OPERATOR_PLUS;
        lexer.ParseToken();//op
        t = lexer.ParseToken();//primary
        current->assign_inst.opernd2_index = location(t.lexeme);

    } else if (t.token_type == MINUS) {
        current->assign_inst.op = OPERATOR_MINUS;
        lexer.ParseToken();//op
        t = lexer.ParseToken();//primary
        current->assign_inst.opernd2_index = location(t.lexeme);

    } else if (t.token_type == MULT) {
        current->assign_inst.op = OPERATOR_MULT;
        lexer.ParseToken();//op
        t = lexer.ParseToken();//primary
        current->assign_inst.opernd2_index = location(t.lexeme);

    } else if (t.token_type == DIV) {
        current->assign_inst.op = OPERATOR_DIV;
        lexer.ParseToken();//op
        t = lexer.ParseToken();//primary
        current->assign_inst.opernd2_index = location(t.lexeme);
    } 

    lexer.ParseToken();//SEMICOLON
}

void parse_stmt() {
    Token t = lexer.peek(1);

    if (t.token_type == INPUT) {
        parse_input_stmt();
    } else if (t.token_type == OUTPUT) {
        parse_output_stmt();
    } else if (t.token_type == ID) {
        parse_assign_stmt();
    } else if (t.token_type == WHILE) {
        parse_while_stmt();
    } else if (t.token_type == IF) {
        parse_if_stmt();
    } else if (t.token_type == SWITCH) {
        parse_switch_stmt();
    } else if (t.token_type == FOR) {
        parse_for_stmt();
    }
}

void parse_stmt_list() {
    parse_stmt();

    Token t = lexer.peek(1);
    if (t.token_type != RBRACE) {
        parse_stmt_list();
    } else {
        current->next = NULL;
    }
}

void parse_body() {
    lexer.ParseToken();//LBRACE
    parse_stmt_list();
    lexer.ParseToken();//RBRACE
} 

void parse_id_list() {
    Token id = lexer.ParseToken(); //ID
    //assign each variable a memory locaion
    variable v;
    v.name = id.lexeme;
    v.address = next_available;
    mem[next_available] = 0;
    variables.push_back(v);
    next_available++;

    //if next token is COMMA
    Token t = lexer.peek(1);
    if (t.token_type == COMMA) {
        lexer.ParseToken(); //COMMA
        parse_id_list();
    }
}

void parse_var_section()  {
    parse_id_list();
    lexer.ParseToken(); //SEMICOLON
}

void parse_program() {
    parse_var_section();
    parse_body();
    parse_inputs();
}

struct * Node Compiler()
{
    parse_program();
    return head;
}
