#include <type_traits>

// ============================================================================
// STREAM
// ============================================================================
template<char... Cs>
struct stream {};


// ============================================================================
// TOKENS
// ============================================================================
struct tok_lpar {};      
struct tok_rpar {};      
struct tok_plus {};      
struct tok_end {};       
template<int N> struct tok_number {};


// ============================================================================
// PARSE NUMBER
// ============================================================================
template<typename Stream, int Acc = 0>
struct parse_number;

// empty
template<int Acc>
struct parse_number<stream<>, Acc> {
    using type = tok_number<Acc>;
    using rest = stream<>;
};

// digit or not
template<char C, char... Cs, int Acc>
struct parse_number<stream<C, Cs...>, Acc> {
    static constexpr bool is_digit = (C >= '0' && C <= '9');
    using next = stream<Cs...>;

    using type =
        std::conditional_t<
            is_digit,
            typename parse_number<next, Acc * 10 + (C - '0')>::type,
            tok_number<Acc>
        >;

    using rest =
        std::conditional_t<
            is_digit,
            typename parse_number<next, Acc * 10 + (C - '0')>::rest,
            stream<C, Cs...>
        >;
};


// ============================================================================
// TOKENIZER
// ============================================================================
template<typename Stream>
struct next_token;

// empty
template<>
struct next_token<stream<>> {
    using type = tok_end;
    using rest = stream<>;
};

// general token
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
// TOKEN STREAM
// ============================================================================
template<typename RawStream>
struct token_stream {
    using tok = next_token<RawStream>;
    using head_token = typename tok::type;
    using rest_stream = typename tok::rest;
};


// ============================================================================
// AST
// ============================================================================
template<int N> struct ast_num {};
template<typename L, typename R> struct ast_add {};


// ============================================================================
// FORWARD DECL
// ============================================================================
template<typename TokenStream>
struct parse_expr;


// ============================================================================
// parse_expr_impl
// ============================================================================
template<typename Tok, typename Rest>
struct parse_expr_impl;

// NUMBER
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
        using expr1 = parse_expr<TS>;
        using R1 = typename expr1::rest;

        static_assert(std::is_same_v<typename R1::head_token, tok_plus>, "Expected '+'");
        using R2 = typename R1::rest_stream;

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
// parse_expr ENTRY
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
// EVALUATOR
// ============================================================================
template<typename Node>
struct eval;

template<int N>
struct eval<ast_num<N>> {
    static constexpr int value = N;
};

template<typename L, typename R>
struct eval<ast_add<L, R>> {
    static constexpr int value = eval<L>::value + eval<R>::value;
};


// ============================================================================
// TEST PROGRAM
// ============================================================================
using program = stream<'(', '3', '+', '5', ')'>;

using tokens = token_stream<program>;
using parsed = parse_expr<tokens>;
using AST = typename parsed::node;

static_assert(eval<AST>::value == 8, "Compile-time evaluation failed!");

int main() {}
