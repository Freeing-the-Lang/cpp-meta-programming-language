#include <type_traits>

//
// ============================================================================
//   C++ TEMPLATE META-PROGRAMMING LANGUAGE — FIXED FULL VERSION
//   (Token → Parser → AST → Eval 컴파일타임 언어)
// ============================================================================
//


// ============================================================================
// 1. STREAM — compile-time char list
// ============================================================================
template<char... Cs>
struct stream {};


// ============================================================================
// 2. TOKENS
// ============================================================================
struct tok_lpar {};      // '('
struct tok_rpar {};      // ')'
struct tok_plus {};      // '+'
struct tok_end {};       // end-of-stream
template<int N> struct tok_number {};


// ============================================================================
// 3. PARSE NUMBER
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

    using next_stream = stream<Cs...>;

    using type = std::conditional_t<
        is_digit,
        typename parse_number<next_stream, Acc * 10 + (C - '0')>::type,
        tok_number<Acc>
    >;

    using rest = std::conditional_t<
        is_digit,
        typename parse_number<next_stream, Acc * 10 + (C - '0')>::rest,
        stream<C, Cs...>
    >;
};


// ============================================================================
// 4. TOKENIZER — single token extraction
// ============================================================================
template<typename Stream>
struct next_token;

// empty stream → end token
template<>
struct next_token<stream<>> {
    using type = tok_end;
    using rest = stream<>;
};

// general case
template<char C, char... Cs>
struct next_token<stream<C, Cs...>> {
    static constexpr bool is_digit = (C >= '0' && C <= '9');
    using number = parse_number<stream<C, Cs...>>;

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
template<typename RawStream>
struct token_stream {
    using tok = next_token<RawStream>;
    using head_token = typename tok::type;
    using rest_stream = typename tok::rest;
};


// ============================================================================
// 6. AST NODES
// ============================================================================
template<int N>
struct ast_num {};

template<typename L, typename R>
struct ast_add {};


// ============================================================================
// 7. PARSER FORWARD DECL.
// ============================================================================
template<typename TokenStream>
struct parse_expr;


// ============================================================================
// 8. parse_expr_impl — number or '('
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

    template<typename TS>
    struct parse_add {
        // 1. parse left expr
        using left_expr = parse_expr<TS>;
        using R1 = typename left_expr::rest;

        // 2. expect '+'
        static_assert(std::is_same_v<typename R1::head_token, tok_plus>, "Expected '+'");
        using R2 = typename R1::rest_stream;

        // 3. parse right expr
        using right_expr = parse_expr<R2>;
        using R3 = typename right_expr::rest;

        // 4. expect ')'
        static_assert(std::is_same_v<typename R3::head_token, tok_rpar>, "Expected ')'");
        using R4 = typename R3::rest_stream;

        using node = ast_add<typename left_expr::node, typename right_expr::node>;
        using rest = R4;
    };

    using impl = parse_add<Rest>;
    using node = typename impl::node;
    using rest = typename impl::rest;
};


// ============================================================================
// 9. parse_expr — entry
// ============================================================================
template<typename TokenStream>
struct parse_expr {
    using tok = typename TokenStream::head_token;
    using rest = typename TokenStream::rest_stream;

    using impl = parse_expr_impl<tok, rest>;
    using node = typename impl::node;
    using rest = typename impl::rest;
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
// 11. TEST PROGRAM — (3+5)
// ============================================================================
using program_str = stream<'(', '3', '+', '5', ')'>;

using tokens = token_stream<program_str>;
using parsed = parse_expr<tokens>;
using AST = typename parsed::node;

static_assert(eval<AST>::value == 8, "Compile-time evaluation failed!");

int main() {}
