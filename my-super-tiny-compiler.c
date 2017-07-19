#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "linked-list.c"

// utils --------------------------------------------------------------

#define printerr(e_type, file, ln, col, txt, ...) \
	fprintf( \
		stderr, \
		"\033[1m\033[37m%s:%zu:%zu: \033[1m\033[31merror:\033[1m\033[37m ‘%s’ \033[0m" txt "\n", \
		file, ln, col, e_type, \
		##__VA_ARGS__ \
	)
#define printerrgene(e_type, txt, ...) \
	fprintf( \
		stderr, \
		"\033[31merror:\033[1m\033[37m ‘%s’ \033[0m" txt "\n", \
		e_type, \
		##__VA_ARGS__ \
	)

#define between(c, a, b) (c >= a && c <= b)

bool cinstr(char c, char *str) {
	for (char *cur = str; *cur != '\0'; ++cur)
		if (c == *cur) return true;
	return false;
}

char* filename = NULL;

// tokenizer ----------------------------------------------------------

typedef enum {
	t_paren,
	t_number,
	t_string,
	t_symbol,
} tok_type;

static const char* tok_type_str[] = {
	"t_paren",
	"t_number",
	"t_string",
	"t_symbol",
};

typedef struct {
	tok_type type;
	char*    val;
	size_t   ln;
	size_t   col;
} token;

token* tok_new(tok_type type, char* val, size_t ln, size_t col) {
	token* self = malloc(sizeof(token));
	self->type = type;
	self->val = val;
	self->ln = ln;
	self->col = col;
	return self;
}

void tok_print(token* self) {
	printf(
		"{ type=%s, val=\"%s\", ln=%zu, col=%zu }\n",
		tok_type_str[self->type],
		self->val, self->ln, self->col
	);
}

char*  tmp_str  = NULL;
size_t tmp_size = 0;
int    tmp_int  = 0;

#define tmp_init() { if (tmp_str == NULL) tmp_str = malloc(sizeof(char) * 4096); }
#define tmp_clear() { tmp_str[0] = '\0'; tmp_size = 0; tmp_int = 0; }

linked_list* /*<token>*/ tokenize(char* input) {
	
	tmp_init();
	
	size_t current = 0;
	linked_list* /*<token>*/ tokens = ll_new();
	
	size_t ln = 1, col = 1, lns = 1, cols = 1;
	char c = input[current];
	
	// add new token
	#define nt(t, v, ln, col) ll_add(tokens, tok_new(t, v, ln, col))
	
	// next character
	#define nxtch() { \
		c = input[++current]; \
		if (c == '\n') ln++, col = 0; else col++; \
	}
	
	// get character around c
	#define getch(i) input[current + (i)]
	
	while (c != '\0') {
	
		if (c == '(') {
			nt(t_paren, "(", ln, col);
			nxtch();
			continue;
		}
		
		if (c == ')') {
			nt(t_paren, ")", ln, col);
			nxtch();
			continue;
		}
		
		if (cinstr(c, " \t\n")) {
			nxtch();
			continue;
		}
		
		#define seek2(check1, check2, t_type, jump)          \
		if (check1) {                                        \
			lns = ln, cols = col;                            \
			tmp_clear();                                     \
			if (jump) nxtch();                               \
			while (check2) {                                 \
				tmp_str[tmp_size++] = c;                     \
				nxtch();                                     \
			}                                                \
			tmp_str[tmp_size] = '\0';                        \
			char* v = malloc(sizeof(char) * (tmp_size + 1)); \
			strcpy(v, tmp_str);                              \
			nt(t_type, v, lns, cols);                        \
			if (jump) nxtch();                               \
			continue;                                        \
		}
		#define seek(check, t_type) seek2(check, check, t_type, false)
		
		seek2(
			between(c, '0', '9'),
			between(c, '0', '9') || cinstr(c, ".xX") ||
				between(c, 'a', 'f') || between(c, 'A', 'F'),
			t_number,
			false
		);
		
		seek2(
			c == '\"',
			c != '\"' || getch(-1) == '\\',
			t_string,
			true
		);
		
		seek(
			between(c, 'A', 'Z') || between(c, 'a', 'z'),
			t_symbol
		);
		
		printerr("unknown_char", filename, ln, col, "%c", c);
		exit(1);
		
	}

	return tokens;
}

// parser -------------------------------------------------------------

typedef enum {
	a_program,
	a_number,
	a_string,
	a_call,
} ast_type;

static const char* ast_type_str[] = {
	"a_program",
	"a_number",
	"a_string",
	"a_call",
};

typedef struct {
	ast_type               type;
	char*                  val;
	linked_list* /*<ast>*/ children;
	size_t                 ln;
	size_t                 col;
} ast;

ast* ast_new(ast_type type, char* val, linked_list* /*<ast>*/ children, size_t ln, size_t col) {
	ast* self = malloc(sizeof(ast));
	self->type = type;
	self->val = val;
	self->children = children;
	self->ln = ln;
	self->col = col;
	return self;
}

#define ast_print(self) _ast_print(self, 0)
void _ast_print(ast* self, unsigned int level) {
	if (self == NULL) {
		printerr("null_pointer_error", __FILE__, (size_t) __LINE__, (size_t) 0, "");
		exit(1);
	}
	for (unsigned int i = 0; i < level; i++) printf("\t");
	printf("{ type=%s, val=\"%s\"", ast_type_str[self->type], self->val);
	if (self->children == NULL) {
		printf(" }\n");
	} else if (self->children->len == 0) {
		printf(", children=[] }\n");
	} else {
		printf(", children=[\n");
		ll_for(ast, a, self->children, _ast_print(a, level + 1));
		for (unsigned int i = 0; i < level; i++) printf("\t");
		printf("]}\n");
	}
}

token*   tok             = NULL;
ll_node* parser_node_cur = NULL;
void next_tok() {
	if (parser_node_cur == NULL) return;
	parser_node_cur = parser_node_cur->next;
	tok = (token*) parser_node_cur == NULL ? NULL : parser_node_cur->item;
}

ast* parser_walk() {
	ast* res = NULL;
	switch (tok->type) {
		case t_number:
			res = ast_new(a_number, tok->val, NULL, tok->ln, tok->col);
			next_tok();
			break;
		case t_string:
			res = ast_new(a_string, tok->val, NULL, tok->ln, tok->col);
			next_tok();
			break;
		case t_paren:
			if (((char*) tok->val)[0] == '(') {
				next_tok();
				if (tok->type != t_symbol) {
					printerr("syntax_error", filename, tok->ln, tok->col, "expected a name");
					exit(1);
				}
				linked_list* /*<ast>*/ ll = ll_new();
				res = ast_new(a_call, tok->val, NULL, tok->ln, tok->col);
				next_tok();
				while (tok && (tok->type != t_paren || (tok->type == t_paren && ((char*) tok->val)[0] != ')'))) {
					ll_add(ll, parser_walk());
				}
				if (ll->len != 0) res->children = ll;
				next_tok();
			} else {
				printerr("syntax_error", filename, tok->ln, tok->col, "expected an opening parenthesis");
				exit(1);
			}
			break;
		default:
			printerr("unknown_token_type_error", filename, tok->ln, tok->col, "token:");
			tok_print(tok);
			exit(1);
	}
	return res;
}

ast* parse(linked_list* /*<token>*/ tokens) {
	parser_node_cur = tokens->first;
	tok = (token*) parser_node_cur->item;
	ast* a = ast_new(a_program, NULL, ll_new(), 0, 0);
	while (parser_node_cur != NULL) {
		ll_add(a->children, parser_walk());
	}
	return a;
}

// traverser ----------------------------------------------------------

#define traverse(self, action) _ast_traverse(self, 0, action)
void _ast_traverse(ast* self, unsigned int depth, void (*action)(ast* item, unsigned int depth, bool has_children)) {
	bool b = self->children != NULL && self->children->len != 0;
	action(self, depth, b);
	if (b)
		ll_for(ast, a, self->children, _ast_traverse(a, depth + 1, action));
}

void ast_traverse_sample(ast* item, unsigned int depth, bool has_children) {
	for (unsigned int i = 0; i < depth; i++) printf("\t");
	printf("type=%s, val=\"%s\"", ast_type_str[item->type], item->val);
	if (has_children) {
		printf(", children=\n");
	} else {
		printf("\n");
	}
}

// generator ----------------------------------------------------------

void generate_code(ast* node, FILE* out) {
	switch (node->type) {
		case a_program:
			ll_for(ast, a, node->children, {
				generate_code(a, out);
				fputs(";\n", out);
			});
			break;
		case a_number:
			fputs(node->val, out);
			break;
		case a_string:
			fputc('\"', out);
			fputs(node->val, out);
			fputc('\"', out);
			break;
		case a_call:
			fputs(node->val, out);
			fputc('(', out);
			ll_for(ast, a, node->children, {
				generate_code(a, out);
				if (__i != node->children->len - 1)
					fputs(", ", out);
			});
			fputc(')', out);
			break;
		default:
			printerr("unknown_ast_type_error", filename, node->ln, node->col, "ast:");
			ast_print(node);
			exit(1);
	}
}

// compiler -----------------------------------------------------------

char* read_text_file(FILE* f) {
	char* buffer = 0;
	size_t length;

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer)
			fread(buffer, 1, length, f);
		fclose(f);
	}
	return buffer;
}

void compiler(FILE* input, FILE* output) {
	char* src = read_text_file(input);
	if (!input) {
		printerrgene("io_error", "could not open file %s", filename);
		exit(1);
	}
	
	linked_list* tokens = tokenize(src);
	//printf("tokenizer:\n"); ll_for(token, t, tokens, tok_print(t));
	
	ast* ast1 = parse(tokens);
	ll_free(tokens);
	//printf("\nparser:\n"); ast_print(ast1);
	
	generate_code(ast1, output);
}

// main ---------------------------------------------------------------

int main(int argc, char** argv) {
	
	filename = argv[1];
	if (!filename) {
		printerrgene("io_error", "no file specified");
		exit(1);
	}
	
	compiler(fopen(filename, "rb"), stdout);
	
	return 0;
}




