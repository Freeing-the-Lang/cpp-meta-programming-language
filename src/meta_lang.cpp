#include <type_traits>

// ============================================================================
// STREAM — compile-time char list
// ============================================================================
template<char... Cs>
struct stream {};


// ============================================================================
// TOKEN DEFINITIONS — Spongelang
// ============================================================================
struct tok_lpar {};
struct tok_rpar {};
struct tok_lbrace {};
struct tok_rbrace {};

struct tok_plus {};
struct tok_minus {};
struct tok_mul {};
struct tok_div {};

struct tok_assign {};
struct tok_comma {};
struct tok_semicolon {};

struct tok_if {};
struct tok_else {};
struct tok_fn {};
struct tok_let {};

template<int N> struct tok_number {};
template<char... Cs> struct tok_ident {
    using chars = tok_ident<Cs...>;
};
template<char... Cs> struct tok_string {
    using chars = tok_string<Cs...>;
};

struct tok_end {};


// ============================================================================
// HELPER: char classification (identifier rules)
// ============================================================================
template<char C>
struct is_alpha {
    static constexpr bool value =
        (C >= 'a' && C <= 'z') ||
        (C >= 'A' && C <= 'Z') ||
        (C == '_');
};

template<char C>
struct is_alnum {
    static constexpr bool value =
        is_alpha<C>::value || (C >= '0' && C <= '9');
};


// ============================================================================
// NUMBER PARSER
// ============================================================================
template<typename Stream, int Acc = 0>
struct parse_number;

template<int Acc>
struct parse_number<stream<>, Acc> {
    using type = tok_number<Acc>;
    using rest = stream<>;
};

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
// IDENTIFIER PARSER
// ============================================================================
template<typename Stream, typename Accum>
struct parse_ident;

template<typename Accum>
struct parse_ident<stream<>, Accum> {
    using type = Accum;
    using rest = stream<>;
};

template<char C, char... Cs, typename Accum>
struct parse_ident<stream<C, Cs...>, Accum> {
    static constexpr bool good = is_alnum<C>::value;

    using type =
        std::conditional_t<
            good,
            typename parse_ident<stream<Cs...>, tok_ident<Accum::chars..., C>>::type,
            Accum
        >;

    using rest =
        std::conditional_t<
            good,
            typename parse_ident<stream<Cs...>, tok_ident<Accum::chars..., C>>::rest,
            stream<C, Cs...>
        >;
};


// ============================================================================
// STRING PARSER — "xxx"
// ============================================================================
template<typename Stream, typename Accum>
struct parse_string;

template<typename Accum>
struct parse_string<stream<>, Accum> {
    static_assert(!sizeof(Accum), "Unterminated string literal");
};

template<char C, char... Cs, typename Accum>
struct parse_string<stream<C, Cs...>, Accum> {
    using tail = stream<Cs...>;

    using type =
        std::conditional_t<
            C == '"',
            tok_string<Accum::chars...>,
            typename parse_string<tail, tok_string<Accum::chars..., C>>::type
        >;

    using rest =
        std::conditional_t<
            C == '"',
            tail,
            typename parse_string<tail, tok_string<Accum::chars..., C>>::rest
        >;
};


// ============================================================================
// KEYWORD MAPPING
// ============================================================================
template<typename Ident>
struct keyword_map {
    using type = Ident;  // default → identifier
};

template<>
struct keyword_map<tok_ident<'i','f'>> {
    using type = tok_if;
};

template<>
struct keyword_map<tok_ident<'e','l','s','e'>> {
    using type = tok_else;
};

template<>
struct keyword_map<tok_ident<'f','n'>> {
    using type = tok_fn;
};

template<>
struct keyword_map<tok_ident<'l','e','t'>> {
    using type = tok_let;
};


// ============================================================================
// MAIN TOKENIZER — next_token
// ============================================================================
template<typename Stream>
struct next_token;

// END
template<>
struct next_token<stream<>> {
    using type = tok_end;
    using rest = stream<>;
};

// Skip whitespace
template<char... Cs>
struct next_token<stream<' ', Cs...>> : next_token<stream<Cs...>> {};

template<char... Cs>
struct next_token<stream<'\n', Cs...>> : next_token<stream<Cs...>> {};

template<char... Cs>
struct next_token<stream<'\t', Cs...>> : next_token<stream<Cs...>> {};

// Main
template<char C, char... Cs>
struct next_token<stream<C, Cs...>> {

    static constexpr bool is_digit_char = (C >= '0' && C <= '9');
    static constexpr bool is_ident_start = is_alpha<C>::value;

    using S = stream<Cs...>;

    // number
    using number = parse_number<stream<C, Cs...>>;

    // identifier
    using ident = parse_ident<stream<C, Cs...>, tok_ident<>>;

    // string "..."
    using string = parse_string<S, tok_string<>>;

    using type =
        std::conditional_t<is_digit_char,
            typename number::type,

        std::conditional_t<is_ident_start,
            typename keyword_map<typename ident::type>::type,

        std::conditional_t<C == '"',
            typename string::type,

        std::conditional_t<C == '(',
            tok_lpar,

        std::conditional_t<C == ')',
            tok_rpar,

        std::conditional_t<C == '{',
            tok_lbrace,

        std::conditional_t<C == '}',
            tok_rbrace,

        std::conditional_t<C == '+',
            tok_plus,

        std::conditional_t<C == '-',
            tok_minus,

        std::conditional_t<C == '*',
            tok_mul,

        std::conditional_t<C == '/',
            tok_div,

        std::conditional_t<C == '=',
            tok_assign,

        std::conditional_t<C == ',',
            tok_comma,

        std::conditional_t<C == ';',
            tok_semicolon,

            tok_end   // fallback
        >>>>>>>>>>>>>>>;

    using rest =
        std::conditional_t<is_digit_char,
            typename number::rest,

        std::conditional_t<is_ident_start,
            typename ident::rest,

        std::conditional_t<C == '"',
            typename string::rest,
            S
        >>>;
};


// ============================================================================
// TOKEN STREAM WRAPPER
// ============================================================================
template<typename RawStream>
struct token_stream {
    using tok = next_token<RawStream>;
    using head_token = typename tok::type;
    using rest_stream = typename tok::rest;
};


// ============================================================================
// TEST: Run tokenizer on small Spongelang snippet
// ============================================================================
using sample =
    stream<
        'l','e','t',' ', 'x',' ','=',' ','3','+','5',';'
    >;

using T0 = token_stream<sample>;
using tok0 = T0::head_token;   // tok_let

static_assert(std::is_same_v<tok0, tok_let>, "Tokenizer failed");

int main() {}
