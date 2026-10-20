#include "c4/yml/parse.hpp"

#ifndef C4_YML_NODE_HPP_
#include "c4/yml/node.hpp"
#endif
#ifndef C4_YML_PARSE_ENGINE_HPP_
#include "c4/yml/parse_engine.hpp"
#endif
#ifndef C4_YML_PARSE_ENGINE_DEF_HPP_
#include "c4/yml/parse_engine.def.hpp"
#endif
#ifndef C4_YML_EVENT_HANDLER_TREE_HPP_
#include "c4/yml/event_handler_tree.hpp"
#endif


//-----------------------------------------------------------------------------

namespace c4 {
namespace yml {


// instantiate the parser class
template class RYML_EXPORT ParseEngine<EventHandlerTree>;


namespace {
// so many things can go wrong...
void check_(Tree *tree)
{
    if C4_UNLIKELY(!tree)
        RYML_ERR_BASIC_("null tree");
    if C4_UNLIKELY(tree->empty())
        tree->reserve();
}
void check_(NodeRef &node)
{
    check_(node.tree());
    if C4_UNLIKELY(node.id() == NONE)
        RYML_ERR_VISIT_CB_(node.tree()->m_callbacks, node.tree(), node.id(), "invalid node");
    node.create();
}
void check_(Parser *parser)
{
    if C4_UNLIKELY(!parser)
        RYML_ERR_BASIC_("null parser");
    if C4_UNLIKELY(!parser->m_evt_handler)
        // the parser callbacks are from the handler. do not use parser->callbacks()
        RYML_ERR_BASIC_("null handler");
}
void check_(Parser *parser, Tree *tree)
{
    if C4_UNLIKELY(!parser && !tree)
    {
        RYML_ERR_BASIC_("null parser and tree");
    }
    else if C4_UNLIKELY(!parser)
    {
        RYML_ERR_BASIC_CB_(tree->callbacks(), "null parser");
    }
    else if C4_UNLIKELY(!tree)
    {
        if C4_UNLIKELY(!parser->m_evt_handler)
            RYML_ERR_BASIC_("null tree and handler");
        else
            RYML_ERR_BASIC_CB_(parser->callbacks(), "null tree");
    }
    if C4_UNLIKELY(!parser->m_evt_handler)
    {
        RYML_ERR_BASIC_("null handler");
    }
    if C4_UNLIKELY(tree->empty())
    {
        tree->reserve();
    }
}
void check_(Parser *parser, NodeRef &node)
{
    check_(parser, node.tree());
    check_(node);
}
void checksrc_(Tree *tree, csubstr src)
{
    if C4_UNLIKELY(src.len && !src.str)
        RYML_ERR_BASIC_CB_(tree->callbacks(), "null source buffer");
}
substr cpsrc_(Tree *tree, csubstr src)
{
    checksrc_(tree, src);
    return tree->copy_to_arena(src);
}
// helpers: check and copy to arena
substr checkcp_(Tree *tree, csubstr src)
{
    check_(tree);
    return cpsrc_(tree, src);
}
substr checkcp_(NodeRef &node, csubstr src)
{
    check_(node);
    return cpsrc_(node.tree(), src);
}
substr checkcp_(Parser *parser, Tree *tree, csubstr src)
{
    check_(parser, tree);
    return cpsrc_(tree, src);
}
substr checkcp_(Parser *parser, NodeRef &node, csubstr src)
{
    check_(parser, node);
    return cpsrc_(node.tree(), src);
}
using Handler = Parser::handler_type;
struct TmpParser
{
    Handler handler;
    Parser  parser;
    // assumes checks above were done prior to instantiation
    TmpParser(ParserOptions const& opts={})
        : handler(get_callbacks())
        , parser(&handler, opts)
    {
    }
    TmpParser(Tree* tree, ParserOptions const& opts={})
        : handler(tree->callbacks())
        , parser(&handler, opts)
    {
    }
    TmpParser(NodeRef &node, ParserOptions const& opts={})
        : handler(node.tree()->callbacks())
        , parser(&handler, opts)
    {
    }
};
// assumes checks above were done prior to calling
C4_ALWAYS_INLINE void reset_handler_(Parser *parser, Tree *tree, id_type node_id)
{
    RYML_ASSERT_BASIC_(parser); // LCOV_EXCL_LINE lcov weirdly fails here
    RYML_ASSERT_BASIC_(parser->m_evt_handler);
    RYML_ASSERT_BASIC_(tree);
    if C4_UNLIKELY(node_id == NONE || node_id >= tree->capacity())
        RYML_ERR_VISIT_CB_(tree->m_callbacks, tree, node_id, "invalid node");
    parser->m_evt_handler->reset(tree, node_id);
    RYML_ASSERT_BASIC_(parser->m_evt_handler->m_tree == tree);
}
// when parsing into a new tree that was not reserved by the caller,
// size the node array from the source so that it does not double
// many times while parsing
C4_ALWAYS_INLINE void reserve_from_source_(Tree *tree, id_type node_id, csubstr src)
{
    if(node_id == tree->root_id()
       && tree->size() == 1
       && tree->capacity() <= RYML_DEFAULT_TREE_CAPACITY
       && tree->type(node_id) == NOTYPE)
    {
        const id_type cap = estimate_tree_capacity(src);
        if(cap > tree->capacity())
            tree->reserve(cap);
    }
}
// assumes checks above were done prior to calling
void parse_yaml_(Parser *parser, csubstr filename, substr yaml, Tree *tree, id_type node_id)
{
    checksrc_(tree, yaml);
    reserve_from_source_(tree, node_id, yaml); // before the handler takes pointers into the tree
    reset_handler_(parser, tree, node_id);
    parser->parse_in_place_ev(filename, yaml);
}
// assumes checks above were done prior to calling
void parse_json_(Parser *parser, csubstr filename, substr json, Tree *tree, id_type node_id)
{
    checksrc_(tree, json);
    reserve_from_source_(tree, node_id, json); // before the handler takes pointers into the tree
    reset_handler_(parser, tree, node_id);
    parser->parse_json_in_place_ev(filename, json);
}
} // namespace



// this is vertically aligned to highlight the parameter differences.
void parse_in_place(Parser *parser, csubstr filename, substr yaml, Tree *tree, id_type node_id) { check_(parser, tree); parse_yaml_(parser, filename, yaml, tree, node_id); }
void parse_in_place(Parser *parser,                   substr yaml, Tree *tree, id_type node_id) { check_(parser, tree); parse_yaml_(parser, {}      , yaml, tree, node_id); }
void parse_in_place(Parser *parser, csubstr filename, substr yaml, Tree *tree                 ) { check_(parser, tree); parse_yaml_(parser, filename, yaml, tree, tree->root_id()); }
void parse_in_place(Parser *parser,                   substr yaml, Tree *tree                 ) { check_(parser, tree); parse_yaml_(parser, {}      , yaml, tree, tree->root_id()); }
void parse_in_place(Parser *parser, csubstr filename, substr yaml, NodeRef node               ) { check_(parser, node); parse_yaml_(parser, filename, yaml, node.tree(), node.id()); }
void parse_in_place(Parser *parser,                   substr yaml, NodeRef node               ) { check_(parser, node); parse_yaml_(parser, {}      , yaml, node.tree(), node.id()); }
Tree parse_in_place(Parser *parser, csubstr filename, substr yaml                             ) { check_(parser); Tree tree(parser->callbacks()); parse_yaml_(parser, filename, yaml, &tree, tree.root_id()); return tree; }
Tree parse_in_place(Parser *parser,                   substr yaml                             ) { check_(parser); Tree tree(parser->callbacks()); parse_yaml_(parser, {}      , yaml, &tree, tree.root_id()); return tree; }

// this is vertically aligned to highlight the parameter differences.
void parse_in_place(csubstr filename, substr yaml, Tree *tree, id_type node_id, ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, filename, yaml, tree, node_id); }
void parse_in_place(                  substr yaml, Tree *tree, id_type node_id, ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, {}      , yaml, tree, node_id); }
void parse_in_place(csubstr filename, substr yaml, Tree *tree                 , ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, filename, yaml, tree, tree->root_id()); }
void parse_in_place(                  substr yaml, Tree *tree                 , ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, {}      , yaml, tree, tree->root_id()); }
void parse_in_place(csubstr filename, substr yaml, NodeRef node               , ParserOptions const& opts) { check_(node); TmpParser tmp(node, opts); parse_yaml_(&tmp.parser, filename, yaml, node.tree(), node.id()); }
void parse_in_place(                  substr yaml, NodeRef node               , ParserOptions const& opts) { check_(node); TmpParser tmp(node, opts); parse_yaml_(&tmp.parser, {}      , yaml, node.tree(), node.id()); }
Tree parse_in_place(csubstr filename, substr yaml                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree;          parse_yaml_(&tmp.parser, filename, yaml, &tree, tree.root_id()); return tree; }
Tree parse_in_place(                  substr yaml                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree;          parse_yaml_(&tmp.parser, {}      , yaml, &tree, tree.root_id()); return tree; }



// this is vertically aligned to highlight the parameter differences.
void parse_json_in_place(Parser *parser, csubstr filename, substr json, Tree *tree, id_type node_id) { check_(parser, tree); parse_json_(parser, filename, json, tree, node_id); }
void parse_json_in_place(Parser *parser,                   substr json, Tree *tree, id_type node_id) { check_(parser, tree); parse_json_(parser, {}      , json, tree, node_id); }
void parse_json_in_place(Parser *parser, csubstr filename, substr json, Tree *tree                 ) { check_(parser, tree); parse_json_(parser, filename, json, tree, tree->root_id()); }
void parse_json_in_place(Parser *parser,                   substr json, Tree *tree                 ) { check_(parser, tree); parse_json_(parser, {}      , json, tree, tree->root_id()); }
void parse_json_in_place(Parser *parser, csubstr filename, substr json, NodeRef node               ) { check_(parser, node); parse_json_(parser, filename, json, node.tree(), node.id()); }
void parse_json_in_place(Parser *parser,                   substr json, NodeRef node               ) { check_(parser, node); parse_json_(parser, {}      , json, node.tree(), node.id()); }
Tree parse_json_in_place(Parser *parser, csubstr filename, substr json                             ) { check_(parser); Tree tree(parser->callbacks()); parse_json_(parser, filename, json, &tree, tree.root_id()); return tree; }
Tree parse_json_in_place(Parser *parser,                   substr json                             ) { check_(parser); Tree tree(parser->callbacks()); parse_json_(parser, {}      , json, &tree, tree.root_id()); return tree; }

// this is vertically aligned to highlight the parameter differences.
void parse_json_in_place(csubstr filename, substr json, Tree *tree, id_type node_id, ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, filename, json, tree, node_id); }
void parse_json_in_place(                  substr json, Tree *tree, id_type node_id, ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, {}      , json, tree, node_id); }
void parse_json_in_place(csubstr filename, substr json, Tree *tree                 , ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, filename, json, tree, tree->root_id()); }
void parse_json_in_place(                  substr json, Tree *tree                 , ParserOptions const& opts) { check_(tree); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, {}      , json, tree, tree->root_id()); }
void parse_json_in_place(csubstr filename, substr json, NodeRef node               , ParserOptions const& opts) { check_(node); TmpParser tmp(node, opts); parse_json_(&tmp.parser, filename, json, node.tree(), node.id()); }
void parse_json_in_place(                  substr json, NodeRef node               , ParserOptions const& opts) { check_(node); TmpParser tmp(node, opts); parse_json_(&tmp.parser, {}      , json, node.tree(), node.id()); }
Tree parse_json_in_place(csubstr filename, substr json                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree;          parse_json_(&tmp.parser, filename, json, &tree, tree.root_id()); return tree; }
Tree parse_json_in_place(                  substr json                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree;          parse_json_(&tmp.parser, {}      , json, &tree, tree.root_id()); return tree; }



// this is vertically aligned to highlight the parameter differences.
void parse_in_arena(Parser *parser, csubstr filename, csubstr yaml, Tree *tree, id_type node_id) { substr src = checkcp_(parser, tree, yaml); parse_yaml_(parser, filename, src, tree, node_id); }
void parse_in_arena(Parser *parser,                   csubstr yaml, Tree *tree, id_type node_id) { substr src = checkcp_(parser, tree, yaml); parse_yaml_(parser, {}      , src, tree, node_id); }
void parse_in_arena(Parser *parser, csubstr filename, csubstr yaml, Tree *tree                 ) { substr src = checkcp_(parser, tree, yaml); parse_yaml_(parser, filename, src, tree, tree->root_id()); }
void parse_in_arena(Parser *parser,                   csubstr yaml, Tree *tree                 ) { substr src = checkcp_(parser, tree, yaml); parse_yaml_(parser, {}      , src, tree, tree->root_id()); }
void parse_in_arena(Parser *parser, csubstr filename, csubstr yaml, NodeRef node               ) { substr src = checkcp_(parser, node, yaml); parse_yaml_(parser, filename, src, node.tree(), node.id()); }
void parse_in_arena(Parser *parser,                   csubstr yaml, NodeRef node               ) { substr src = checkcp_(parser, node, yaml); parse_yaml_(parser, {}      , src, node.tree(), node.id()); }
Tree parse_in_arena(Parser *parser, csubstr filename, csubstr yaml                             ) { check_(parser); Tree tree(parser->callbacks()); substr src = cpsrc_(&tree, yaml); parse_yaml_(parser, filename, src, &tree, tree.root_id()); return tree; }
Tree parse_in_arena(Parser *parser,                   csubstr yaml                             ) { check_(parser); Tree tree(parser->callbacks()); substr src = cpsrc_(&tree, yaml); parse_yaml_(parser, {}      , src, &tree, tree.root_id()); return tree; }

// this is vertically aligned to highlight the parameter differences.
void parse_in_arena(csubstr filename, csubstr yaml, Tree *tree, id_type node_id, ParserOptions const& opts) { substr src = checkcp_(tree, yaml); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, filename, src, tree, node_id); }
void parse_in_arena(                  csubstr yaml, Tree *tree, id_type node_id, ParserOptions const& opts) { substr src = checkcp_(tree, yaml); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, {}      , src, tree, node_id); }
void parse_in_arena(csubstr filename, csubstr yaml, Tree *tree                 , ParserOptions const& opts) { substr src = checkcp_(tree, yaml); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, filename, src, tree, tree->root_id()); }
void parse_in_arena(                  csubstr yaml, Tree *tree                 , ParserOptions const& opts) { substr src = checkcp_(tree, yaml); TmpParser tmp(tree, opts); parse_yaml_(&tmp.parser, {}      , src, tree, tree->root_id()); }
void parse_in_arena(csubstr filename, csubstr yaml, NodeRef node               , ParserOptions const& opts) { substr src = checkcp_(node, yaml); TmpParser tmp(node, opts); parse_yaml_(&tmp.parser, filename, src, node.tree(), node.id()); }
void parse_in_arena(                  csubstr yaml, NodeRef node               , ParserOptions const& opts) { substr src = checkcp_(node, yaml); TmpParser tmp(node, opts); parse_yaml_(&tmp.parser, {}      , src, node.tree(), node.id()); }
Tree parse_in_arena(csubstr filename, csubstr yaml                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree; substr src = cpsrc_(&tree, yaml); parse_yaml_(&tmp.parser, filename, src, &tree, tree.root_id()); return tree; }
Tree parse_in_arena(                  csubstr yaml                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree; substr src = cpsrc_(&tree, yaml); parse_yaml_(&tmp.parser, {}      , src, &tree, tree.root_id()); return tree; }



// this is vertically aligned to highlight the parameter differences.
void parse_json_in_arena(Parser *parser, csubstr filename, csubstr json, Tree *tree, id_type node_id) { substr src = checkcp_(parser, tree, json); parse_json_(parser, filename, src, tree, node_id); }
void parse_json_in_arena(Parser *parser,                   csubstr json, Tree *tree, id_type node_id) { substr src = checkcp_(parser, tree, json); parse_json_(parser, {}      , src, tree, node_id); }
void parse_json_in_arena(Parser *parser, csubstr filename, csubstr json, Tree *tree                 ) { substr src = checkcp_(parser, tree, json); parse_json_(parser, filename, src, tree, tree->root_id()); }
void parse_json_in_arena(Parser *parser,                   csubstr json, Tree *tree                 ) { substr src = checkcp_(parser, tree, json); parse_json_(parser, {}      , src, tree, tree->root_id()); }
void parse_json_in_arena(Parser *parser, csubstr filename, csubstr json, NodeRef node               ) { substr src = checkcp_(parser, node, json); parse_json_(parser, filename, src, node.tree(), node.id()); }
void parse_json_in_arena(Parser *parser,                   csubstr json, NodeRef node               ) { substr src = checkcp_(parser, node, json); parse_json_(parser, {}      , src, node.tree(), node.id()); }
Tree parse_json_in_arena(Parser *parser, csubstr filename, csubstr json                             ) { check_(parser); Tree tree(parser->callbacks()); substr src = cpsrc_(&tree, json); parse_json_(parser, filename, src, &tree, tree.root_id()); return tree; }
Tree parse_json_in_arena(Parser *parser,                   csubstr json                             ) { check_(parser); Tree tree(parser->callbacks()); substr src = cpsrc_(&tree, json); parse_json_(parser, {}      , src, &tree, tree.root_id()); return tree; }

// this is vertically aligned to highlight the parameter differences.
void parse_json_in_arena(csubstr filename, csubstr json, Tree *tree, id_type node_id, ParserOptions const& opts) { substr src = checkcp_(tree, json); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, filename, src, tree, node_id); }
void parse_json_in_arena(                  csubstr json, Tree *tree, id_type node_id, ParserOptions const& opts) { substr src = checkcp_(tree, json); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, {}      , src, tree, node_id); }
void parse_json_in_arena(csubstr filename, csubstr json, Tree *tree                 , ParserOptions const& opts) { substr src = checkcp_(tree, json); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, filename, src, tree, tree->root_id()); }
void parse_json_in_arena(                  csubstr json, Tree *tree                 , ParserOptions const& opts) { substr src = checkcp_(tree, json); TmpParser tmp(tree, opts); parse_json_(&tmp.parser, {}      , src, tree, tree->root_id()); }
void parse_json_in_arena(csubstr filename, csubstr json, NodeRef node               , ParserOptions const& opts) { substr src = checkcp_(node, json); TmpParser tmp(node, opts); parse_json_(&tmp.parser, filename, src, node.tree(), node.id()); }
void parse_json_in_arena(                  csubstr json, NodeRef node               , ParserOptions const& opts) { substr src = checkcp_(node, json); TmpParser tmp(node, opts); parse_json_(&tmp.parser, {}      , src, node.tree(), node.id()); }
Tree parse_json_in_arena(csubstr filename, csubstr json                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree; substr src = cpsrc_(&tree, json); parse_json_(&tmp.parser, filename, src, &tree, tree.root_id()); return tree; }
Tree parse_json_in_arena(                  csubstr json                             , ParserOptions const& opts) { TmpParser tmp(opts); Tree tree; substr src = cpsrc_(&tree, json); parse_json_(&tmp.parser, {}      , src, &tree, tree.root_id()); return tree; }



//-----------------------------------------------------------------------------

namespace {
// the high bit of each byte of w which equals c
C4_ALWAYS_INLINE uint64_t bytes_eq_(uint64_t w, uint64_t c) noexcept
{
    const uint64_t lo7 = UINT64_C(0x7f7f7f7f7f7f7f7f);
    const uint64_t x = w ^ (c * UINT64_C(0x0101010101010101));
    return ~(((x & lo7) + lo7) | x | lo7);
}
// the number of bytes with the high bit set, in a mask from bytes_eq_()
C4_ALWAYS_INLINE id_type count_bytes_(uint64_t m) noexcept
{
    return (id_type)(((m >> 7) * UINT64_C(0x0101010101010101)) >> 56);
}
} // namespace

RYML_EXPORT id_type estimate_tree_capacity(csubstr src)
{
    // count one node per '\n', ',', '[' and '{', and one more per
    // block sequence entry ("- " or "-\n"), which opens a node on a
    // line that often holds another one, eg "- name: foo"
    id_type num_nodes = 1; // root
    size_t i = 0;
    #if C4_BYTE_ORDER == C4EL_
    // eight bytes at a time
    uint64_t dash_carry = 0; // the last byte of the previous word was '-'
    for( ; i + 8 <= src.len; i += 8)
    {
        uint64_t w;
        memcpy(&w, src.str + i, 8);
        const uint64_t nl = bytes_eq_(w, '\n');
        const uint64_t sp = bytes_eq_(w, ' ') | nl;
        const uint64_t dash = bytes_eq_(w, '-');
        num_nodes += count_bytes_(nl | bytes_eq_(w, ',') | bytes_eq_(w, '[') | bytes_eq_(w, '{'));
        num_nodes += count_bytes_(((dash << 8) | dash_carry) & sp);
        dash_carry = (dash >> 56) & UINT64_C(0x80);
    }
    if(dash_carry && i < src.len && (src.str[i] == ' ' || src.str[i] == '\n'))
        ++num_nodes;
    #endif
    for( ; i < src.len; ++i)
    {
        const char c = src.str[i];
        num_nodes += (c == '\n') || (c == ',') || (c == '[') || (c == '{');
        num_nodes += (c == '-') && (i + 1 < src.len) && (src.str[i + 1] == ' ' || src.str[i + 1] == '\n');
    }
    return num_nodes;
}

} // namespace yml
} // namespace c4
