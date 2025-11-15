#include <type_traits>

//
// ============================================================================
//   C++ TEMPLATE META-PROGRAMMING LANGUAGE (FULL VERSION, ONE FILE)
//   - Tokenizer
//   - Stream
//   - Grammar
//   - Parser
//   - AST
//   - Evaluator
//   - Compile-Time Execution
// ============================================================================
//


// ============================================================================
// 1. STRING → STREAM
// ============================================================================
template<char... Cs>
struct stream {
    static constexpr bool empty = false;
};

template<>
struct stream<> {
    static constexpr bool empty = true;
};


// ============================================================================
// 2. TOKEN DEFINITIONS
// ============================================================================
struct tok_lpar {};      // '('
struct tok_rpar {};      // ')'
struct tok_plus {};      // '+'
struct tok_end {};       // end
template<int N> struct tok_number {};


// ============================================================================
// 3. PARSE NUMBER (recursive template)
// ============================================================================
template<typename Stream, int Acc = 0>
struct parse_number;

// empty
template<int Acc>
struct parse_number<stream<>, Acc> {
    using type = tok_number<Acc>;
    using rest = stream<>;
};

// digit case
template<char C, char... Cs, int Acc>
struct parse_number<stream<C, Cs...>, Acc> {
    static constexpr bool is_digit = (C >= '0' && C <= '9');

    using next = stream<Cs...>;

    using type = std::conditional_t<
        is_digit,
        typename parse_number<next, Acc * 10 + (C - '0')>::type,
        tok_number<Acc>
    >;

    using rest = std::conditional_t<
        is_digit,
        typename parse_number<next, Acc * 10 + (C - '0')>::rest,
        stream<C, Cs...>
    >;
};


// ============================================================================
// 4. TOKENIZER
// ============================================================================
template<typename Stream>
struct next_token;

// empty
template<>
struct next_token<stream<>> {
    using type = tok_end;
    using rest = stream<>;
};

// general case
template<char C, char... Cs>
struct next_token<stream<C, Cs...>> {
    using number = parse_number<stream<C, Cs...>>;
    static constexpr bool is_digit = (C >= '0' && C <= '9');

    using type = std::conditional_t<
        is_digit,
        typename number::type,
        std::conditional_t<
            C == '(', tok_lpar,
            std::conditional_t<
                C == ')', tok_rpar,
                std::conditional_t<
                    C == '+', tok_plus,
                    tok_end
                >
            >
        >
    >;

    using rest = std::conditional_t<
        is_digit,
        typename number::rest,
        stream<Cs...>
    >;
};


// ============================================================================
// 5. TOKEN STREAM WRAPPER
// ============================================================================
template<typename Stream>
struct token_stream {
    using tok = next_token<Stream>;
    using head_token = typename tok::type;
    using rest_stream = typename tok::rest;
};


// ============================================================================
// 6. AST DEFINITIONS
// ============================================================================
template<int N>
struct ast_num {};

template<typename L, typename R>
struct ast_add {};


// ============================================================================
// 7. PARSER FORWARD DECL
// ============================================================================
template<typename TokenStream>
struct parse_expr;


// ============================================================================
// 8. parse_expr_impl (숫자 or '(' 시작)
// ============================================================================
template<typename Tok, typename Rest>
struct parse_expr_impl;

// number
template<int N, typename Rest>
struct parse_expr_impl<tok_number<N>, Rest> {
    using node = ast_num<N>;
    using rest = Rest;
};

// '(' Expr '+' Expr ')'
template<typename Rest>
struct parse_expr_impl<tok_lpar, Rest> {
    // Add parser
    template<typename TS>
    struct parse_add {
        // Expr1
        using expr1 = parse_expr<TS>;
        using R1 = typename expr1::rest;

        static_assert(std::is_same_v<typename R1::head_token, tok_plus>, "Expected '+'");
        using R2 = typename R1::rest_stream;

        // Expr2
        using expr2 = parse_expr<R2>;
        using R3 = typename expr2::rest;

        static_assert(std::is_same_v<typename R3::head_token, tok_rpar>, "Expected ')'");
        using R4 = typename R3::rest_stream;

        using node = ast_add<typename expr1::node, typename expr2::node>;
        using rest = R4;
    };

    using impl = parse_add<Rest>;
    using node = typename impl::node;
    using rest = typename impl::rest;
};


// ============================================================================
// 9. parse_expr (최종 parser)
// ============================================================================
template<typename TokenStream>
struct parse_expr {
    using tok = typename TokenStream::head_token;
    using rest = typename TokenStream::rest_stream;

    using impl = parse_expr_impl<tok, rest>;
    using node = typename impl::node;
    using rest_final = typename impl::rest;
};


// ============================================================================
// 10. EVALUATOR
// ============================================================================
template<typename Node>
struct eval;

// number
template<int N>
struct eval<ast_num<N>> {
    static constexpr int value = N;
};

// add
template<typename L, typename R>
struct eval<ast_add<L, R>> {
    static constexpr int value = eval<L>::value + eval<R>::value;
};


// ============================================================================
// 11. TEST PROGRAM (COMPILE-TIME EXECUTION)
// ============================================================================
//
// Example: (3+5)
//
using program_str = stream<'(', '3', '+', '5', ')'>;

using tokens = token_stream<program_str>;
using parsed = parse_expr< tokens >;
using AST = typename parsed::node;

static_assert(eval<AST>::value == 8, "Compile-time evaluation failed!");

int main() {}
