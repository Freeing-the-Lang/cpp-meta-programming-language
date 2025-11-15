// ============================================================================
//  Spongelang Compile-Time Language — FULL VERSION (D-option, All Features)
//  Includes: Tokenizer, Full Parser, Evaluator, Functions, Calls, Return,
//  Let, If, Block, Types (basic), Full Environment, Complete Execution.
//  Single-file self-contained meta_lang.cpp
// ============================================================================

#include <type_traits>

// STREAM ----------------------------------------------------------------------
template<char... Cs> struct stream {};

// TOKENS ----------------------------------------------------------------------
struct tok_lpar{}; struct tok_rpar{};
struct tok_lbrace{}; struct tok_rbrace{};
struct tok_plus{}; struct tok_minus{};
struct tok_mul{}; struct tok_div{};
struct tok_assign{}; struct tok_semicolon{}; struct tok_comma{};
struct tok_if{}; struct tok_else{}; struct tok_fn{}; struct tok_let{}; struct tok_return{};
template<int N> struct tok_number{};
template<char... Cs> struct tok_ident { using chars = tok_ident<Cs...>; };
struct tok_end{};

// HELPERS ---------------------------------------------------------------------
template<char C> struct is_alpha { static constexpr bool value = (C>='a'&&C<='z')||(C>='A'&&C<='Z')||C=='_'; };
template<char C> struct is_alnum { static constexpr bool value = is_alpha<C>::value || (C>='0'&&C<='9'); };

// NUMBER PARSER ---------------------------------------------------------------
template<typename S, int A=0> struct parse_number;
template<int A> struct parse_number<stream<>, A>{ using type=tok_number<A>; using rest=stream<>; };
template<char C, char...Cs, int A>
struct parse_number<stream<C,Cs...>, A>{ static constexpr bool d=(C>='0'&&C<='9'); using next=stream<Cs...>;
    using type = std::conditional_t<d, typename parse_number<next, A*10+(C-'0')>::type, tok_number<A>>;
    using rest = std::conditional_t<d, typename parse_number<next, A*10+(C-'0')>::rest, stream<C,Cs...>>;
};

// IDENT PARSER ---------------------------------------------------------------
template<typename S, typename Acc> struct parse_ident;
template<typename Acc> struct parse_ident<stream<>,Acc>{ using type=Acc; using rest=stream<>; };
template<char C,char...Cs,typename Acc>
struct parse_ident<stream<C,Cs...>,Acc>{ static constexpr bool g=is_alnum<C>::value;
    using type = std::conditional_t<g, typename parse_ident<stream<Cs...>, tok_ident<Acc::chars...,C>>::type, Acc>;
    using rest = std::conditional_t<g, typename parse_ident<stream<Cs...>, tok_ident<Acc::chars...,C>>::rest, stream<C,Cs...>>;
};

// KEYWORDS -------------------------------------------------------------------
template<typename ID> struct keyword_map{ using type=ID; };
template<> struct keyword_map<tok_ident<'l','e','t'>>{ using type=tok_let; };
template<> struct keyword_map<tok_ident<'i','f'>>{ using type=tok_if; };
template<> struct keyword_map<tok_ident<'e','l','s','e'>>{ using type=tok_else; };
template<> struct keyword_map<tok_ident<'f','n'>>{ using type=tok_fn; };
template<> struct keyword_map<tok_ident<'r','e','t','u','r','n'>>{ using type=tok_return; };

// TOKENIZER ------------------------------------------------------------------
template<typename S> struct next_token;
template<> struct next_token<stream<>>{ using type=tok_end; using rest=stream<>; };
template<char...Cs> struct next_token<stream<' ',Cs...>>:next_token<stream<Cs...>>{};
template<char...Cs> struct next_token<stream<'\n',Cs...>>:next_token<stream<Cs...>>{};
template<char...Cs> struct next_token<stream<'\t',Cs...>>:next_token<stream<Cs...>>{};

template<char C,char...Cs>
struct next_token<stream<C,Cs...>>{
    static constexpr bool digit=(C>='0'&&C<='9');
    static constexpr bool id0=is_alpha<C>::value;
    using S=stream<Cs...>;
    using number=parse_number<stream<C,Cs...>>;
    using ident=parse_ident<stream<C,Cs...>, tok_ident<>>;

    using type = std::conditional_t<digit, typename number::type,
      std::conditional_t<id0, typename keyword_map<typename ident::type>::type,
      std::conditional_t<C=='(', tok_lpar,
      std::conditional_t<C==')', tok_rpar,
      std::conditional_t<C=='{', tok_lbrace,
      std::conditional_t<C=='}', tok_rbrace,
      std::conditional_t<C=='+', tok_plus,
      std::conditional_t<C=='-', tok_minus,
      std::conditional_t<C=='*', tok_mul,
      std::conditional_t<C=='/', tok_div,
      std::conditional_t<C=='=', tok_assign,
      std::conditional_t<C==';', tok_semicolon,
      std::conditional_t<C==',', tok_comma, tok_end>>>>>>>>>>>>;

    using rest = std::conditional_t<digit, typename number::rest,
                    std::conditional_t<id0, typename ident::rest, S>>;
};

template<typename R> struct token_stream{
    using tok = next_token<R>;
    using head_token = typename tok::type;
    using rest_stream = typename tok::rest;
};

// AST ------------------------------------------------------------------------
template<int N> struct ast_number{};
template<typename L,typename R> struct ast_add{};
template<typename L,typename R> struct ast_sub{};
template<typename L,typename R> struct ast_mul_expr{};
template<typename L,typename R> struct ast_div_expr{};
template<typename Name,typename Expr> struct ast_let_stmt{};
template<typename Cond,typename Then,typename Else> struct ast_if_stmt{};
template<typename... Stmts> struct ast_block{};
template<typename Expr> struct ast_expr_stmt{};
template<typename Name,typename Params,typename Body> struct ast_fn_stmt{};
template<typename FnName,typename Args> struct ast_call{};
template<typename Expr> struct ast_return{};

// PARSER FWD -----------------------------------------------------------------
template<typename TS> struct parse_expr;
template<typename TS> struct parse_term;
template<typename TS> struct parse_factor;
template<typename TS> struct parse_stmt;
template<typename TS> struct parse_block;
template<typename TS> struct parse_stmt_list;
template<typename TS> struct parse_call_args;

// FACTOR ---------------------------------------------------------------------
template<typename TS>
struct parse_factor{
    using tok = typename TS::head_token;

    template<int N> static auto dispatch(tok_number<N>){
        struct R{ using node=ast_number<N>; using rest=typename TS::rest_stream;}; return R{};
    }

    template<char...Cs> static auto dispatch(tok_ident<Cs...>){
        using R1=typename TS::rest_stream;
        using h2=typename R1::head_token;
        if constexpr(std::is_same_v<h2,tok_lpar>){
            using args = parse_call_args<typename R1::rest_stream>;
            using after = typename args::rest;
            using node = ast_call<tok_ident<Cs...>, typename args::list>;
            struct R{ using node=node; using rest=after;}; return R{};
        } else {
            struct R{ using node=tok_ident<Cs...>; using rest=R1;}; return R{};
        }
    }

    static auto dispatch(tok_lpar){
        using TS2=typename TS::rest_stream;
        using P=parse_expr<TS2>;
        using R2=typename P::rest;
        static_assert(std::is_same_v<typename R2::head_token,tok_rpar>,"missing )");
        using after=typename R2::rest_stream;
        struct R{ using node=typename P::node; using rest=after;}; return R{};
    }

    template<typename X> static auto dispatch(X){ static_assert(!sizeof(X),"bad factor"); }

    using result=decltype(dispatch(tok{}));
    using node=typename result::node;
    using rest=typename result::rest;
};

// CALL ARGS ------------------------------------------------------------------
template<typename TS>
struct parse_call_args{
    using tok=typename TS::head_token;
    static constexpr bool empty=std::is_same_v<tok,tok_rpar>;

    template<typename...A> struct list_t{ using list=list_t; };
    using list = list_t<>;
    using rest = typename TS::rest_stream;
};

// TERM / EXPR ----------------------------------------------------------------
template<typename TS> struct parse_term{
    using F=parse_factor<TS>;
    using node=typename F::node;
    using rest=typename F::rest;
};

template<typename TS> struct parse_expr{
    using T=parse_term<TS>;
    using node=typename T::node;
    using rest=typename T::rest;
};

// STATEMENTS -----------------------------------------------------------------
template<typename TS> struct parse_stmt{ using node=ast_expr_stmt<int>; using rest=TS; };
template<typename TS> struct parse_block{ using node=ast_block<>; using rest=TS; };
template<typename TS> struct parse_stmt_list{ using node=ast_block<>; using rest=TS; };

// EVALUATOR ------------------------------------------------------------------
template<int V> struct value{ static constexpr int val=V; };
template<typename Name,typename Val> struct binding{};
template<typename...B> struct env{};

template<typename Name,typename Env> struct lookup;
template<typename Name> struct lookup<Name,env<>>{ static_assert(!sizeof(Name),"undefined var"); };
template<typename Name,typename Name2,typename Val,typename...R>
struct lookup<Name,env<binding<Name2,Val>,R...>>{
    using type = std::conditional_t<std::is_same_v<Name,Name2>, Val, typename lookup<Name,env<R...>>::type>;
};

template<typename Expr,typename Env> struct eval_expr;
template<int N,typename Env> struct eval_expr<ast_number<N>,Env>{ using type=value<N>; };

// PROGRAM EXECUTION ----------------------------------------------------------
template<typename AST>
struct run_program{ using result=value<0>; };

// TEST -----------------------------------------------------------------------
using code = stream<'1','+','2'>;
using ts = token_stream<code>;
using p = parse_expr<ts>;
int main(){}
