/* must: simple assertion library */

/* must(): ensures that the given expression is true;
 * otherwise, prints an error message and exits. */
#define must(must_be_true, /* printf args (PASS AT LEAST ONE!) */...)\
	must_(must_be_true, __FILE__, __LINE__, __func__, __VA_ARGS__);

/* Internal function, use must() instead. */
#define must_(must_be_true, file, line, func, ...)\
	if (!(must_be_true)) {\
		fprintf(stderr, "\033[31;1mRuntime error\033[0m in function '%s': %s:%d: ",\
				func, file, line);\
		fprintf(stderr, __VA_ARGS__);\
		fputc('\n', stderr);\
		exit(1);\
	}
