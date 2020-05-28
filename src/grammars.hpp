/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#pragma once

#include <boost/spirit/home/x3.hpp>
#include "ast_commands.hpp"
#include "ast_handlers.hpp"
#include "common_parsers.hpp"
#include "leaf_data.hpp"
#include "path_parser.hpp"


x3::rule<discard_class, discard_> const discard = "discard";
x3::rule<ls_class, ls_> const ls = "ls";
x3::rule<cd_class, cd_> const cd = "cd";
x3::rule<set_class, set_> const set = "set";
x3::rule<get_class, get_> const get = "get";
x3::rule<create_class, create_> const create = "create";
x3::rule<delete_class, delete_> const delete_rule = "delete_rule";
x3::rule<commit_class, commit_> const commit = "commit";
x3::rule<describe_class, describe_> const describe = "describe";
x3::rule<help_class, help_> const help = "help";
x3::rule<copy_class, copy_> const copy = "copy";
x3::rule<move_class, move_> const move = "move";
x3::rule<command_class, command_> const command = "command";

x3::rule<createCommandSuggestions_class, x3::unused_type> const createCommandSuggestions = "createCommandSuggestions";

#if __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif

namespace ascii = boost::spirit::x3::ascii;

struct ls_options_table : x3::symbols<LsOption> {
    ls_options_table()
    {
    add
        ("--recursive", LsOption::Recursive);
    }
} const ls_options;

auto const ls_def =
    ls_::name >> *(space_separator >> ls_options) >> -(space_separator >> (anyPath | (module >> "*")));

auto const cd_def =
    cd_::name >> space_separator > dataPath;

auto const create_def =
    create_::name >> space_separator > (presenceContainerPath | listInstancePath | leafListElementPath);

auto const delete_rule_def =
    delete_::name >> space_separator > (presenceContainerPath | listInstancePath | leafListElementPath);

auto const get_def =
    get_::name >> -(space_separator >> ((dataPathListEnd | dataPath) | (module >> "*")));

auto const set_def =
    set_::name >> space_separator > writableLeafPath > space_separator > leaf_data;

auto const commit_def =
    commit_::name >> x3::attr(commit_());

auto const discard_def =
    discard_::name >> x3::attr(discard_());

struct command_names_table : x3::symbols<decltype(help_::m_cmd)> {
    command_names_table()
    {
        boost::mpl::for_each<CommandTypes, boost::type<boost::mpl::_>>([this](auto cmd) {
            add(commandNamesVisitor(cmd), decltype(help_::m_cmd)(cmd));
        });
    }
} const command_names;

auto const help_def =
    help_::name > createCommandSuggestions >> -command_names;

struct datastore_symbol_table : x3::symbols<Datastore> {
    datastore_symbol_table()
    {
        add
            ("running", Datastore::Running)
            ("startup", Datastore::Startup);
    }
} const datastore;

auto copy_source = x3::rule<class source, Datastore>{"source datastore"} = datastore;
auto copy_destination = x3::rule<class source, Datastore>{"destination datastore"} = datastore;

const auto datastoreSuggestions = x3::eps[([](auto& ctx) {
    auto& parserContext = x3::get<parser_context_tag>(ctx);
    parserContext.m_suggestions = {Completion{"running", " "}, Completion{"startup", " "}};
    parserContext.m_completionIterator = _where(ctx).begin();
})];

struct copy_args : x3::parser<copy_args> {
    using attribute_type = copy_;
    template <typename It, typename Ctx, typename RCtx>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, copy_& attr) const
    {
        auto& parserContext = x3::get<parser_context_tag>(ctx);
        auto iterBeforeDestination = begin;
        auto save_iter = x3::no_skip[x3::eps[([&iterBeforeDestination](auto& ctx) {iterBeforeDestination = _where(ctx).begin();})]];
        auto grammar = datastoreSuggestions > copy_source > space_separator > datastoreSuggestions > save_iter > copy_destination;

        try {
            grammar.parse(begin, end, ctx, rctx, attr);
        } catch (x3::expectation_failure<It>& ex) {
            using namespace std::string_literals;
            parserContext.m_errorMsg = "Expected "s + ex.which() + " here:";
            throw;
        }

        if (attr.m_source == attr.m_destination) {
            begin = iterBeforeDestination; // Restoring the iterator here makes the error caret point to the second datastore
            parserContext.m_errorMsg = "Source datastore and destination datastore can't be the same.";
            return false;
        }

        return true;
    }
} copy_args;

auto const copy_def =
    copy_::name > space_separator > copy_args;

auto const describe_def =
    describe_::name >> space_separator > (dataPathListEnd | anyPath);

struct mode_table : x3::symbols<MoveMode> {
    mode_table()
    {
        add
            ("after", MoveMode::After)
            ("before", MoveMode::Before)
            ("begin", MoveMode::Begin)
            ("end", MoveMode::End);
    }
} const mode_table;

struct move_absolute_table : x3::symbols<Absolute> {
    move_absolute_table()
    {
        add
            ("begin", Absolute::Begin)
            ("end", Absolute::End);
    }
} const move_absolute_table;

struct move_relative_table : x3::symbols<Relative::Position> {
    move_relative_table()
    {
        add
            ("before", Relative::Position::Before)
            ("after", Relative::Position::After);
    }
} const move_relative_table;

struct move_args : x3::parser<move_args> {
    using attribute_type = move_;
    template <typename It, typename Ctx, typename RCtx>
    bool parse(It& begin, It end, Ctx const& ctx, RCtx& rctx, move_& attr) const
    {
        ParserContext& parserContext = x3::get<parser_context_tag>(ctx);
        dataPath_ movePath;
        auto movePathGrammar = listInstancePath | leafListElementPath;
        auto res = movePathGrammar.parse(begin, end, ctx, rctx, attr.m_source);
        if (!res) {
            parserContext.m_errorMsg = "Expected source path here:";
            return false;
        }

        // Try absolute position first.
        res = (space_separator >> move_absolute_table).parse(begin, end, ctx, rctx, attr.m_destination);
        if (res) {
            // If absolute position succeeded, we're done.
            return true;
        }

        // If absolute position didn't succeed, try relative.
        attr.m_destination = Relative{};
        res = (space_separator >> move_relative_table).parse(begin, end, ctx, rctx, std::get<Relative>(attr.m_destination).m_position);

        if (!res) {
            parserContext.m_errorMsg = "Expected a move position (begin, end, before, after) here:";
            return false;
        }

        if (std::holds_alternative<leafListElement_>(attr.m_source.m_nodes.back().m_suffix)) {
            leaf_data_ value;
            res = (space_separator >> leaf_data).parse(begin, end, ctx, rctx, value);
            if (res) {
                std::get<Relative>(attr.m_destination).m_path = {{".", value}};
            }
        } else {
            ListInstance listInstance;
            // The source list instance will be stored inside the parser
            // context. However, m_tmpListPath expects a path ending with a
            // list_. I have to swap the listElement_ to a list_.
            parserContext.m_tmpListPath = parserContext.currentDataPath();
            parserContext.m_tmpListPath.m_nodes.pop_back();
            auto list = list_{std::get<listElement_>(attr.m_source.m_nodes.back().m_suffix).m_name};
            parserContext.m_tmpListPath.m_nodes.push_back(dataNode_{attr.m_source.m_nodes.back().m_prefix, list});

            res = (space_separator >> listSuffix).parse(begin, end, ctx, rctx, listInstance);
            if (res) {
                std::get<Relative>(attr.m_destination).m_path = listInstance;
            }
        }

        if (!res) {
            parserContext.m_errorMsg = "Expected a destination here:";
        }

        return res;
    }
} const move_args;

auto const move_def =
    move_::name >> space_separator >> move_args;

auto const createCommandSuggestions_def =
    x3::eps;

auto const command_def =
    createCommandSuggestions >> x3::expect[cd | copy | create | delete_rule | set | commit | get | ls | discard | describe | help | move];

#if __clang__
#pragma GCC diagnostic pop
#endif

BOOST_SPIRIT_DEFINE(set)
BOOST_SPIRIT_DEFINE(commit)
BOOST_SPIRIT_DEFINE(get)
BOOST_SPIRIT_DEFINE(ls)
BOOST_SPIRIT_DEFINE(discard)
BOOST_SPIRIT_DEFINE(cd)
BOOST_SPIRIT_DEFINE(create)
BOOST_SPIRIT_DEFINE(delete_rule)
BOOST_SPIRIT_DEFINE(describe)
BOOST_SPIRIT_DEFINE(help)
BOOST_SPIRIT_DEFINE(copy)
BOOST_SPIRIT_DEFINE(move)
BOOST_SPIRIT_DEFINE(command)
BOOST_SPIRIT_DEFINE(createCommandSuggestions)
