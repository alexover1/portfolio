/*
 * Generates HTML from a list of Markdown files.
 */

#include "md.h"
#include "md4c-html.h"

static MD_Arena *arena = 0;

static void
MD_SaveEntireFile(MD_Arena *arena, MD_String8 str, MD_String8 filename)
{
    MD_ArenaTemp scratch = MD_GetScratch(&arena, 1);
    MD_String8 file_contents = MD_ZERO_STRUCT;
    MD_String8 filename_copy = MD_S8Copy(scratch.arena, filename);
    FILE *file = fopen((char*)filename_copy.str, "wb");
    if(file != 0)
    {
        fwrite(str.str, str.size, 1, file);
        fwrite("\n", 1, 1, file);
        fclose(file);
    }
    MD_ReleaseScratch(scratch);
}

static void
ProcessHTMLChunk(const MD_CHAR *str, MD_SIZE size, void *user)
{
    MD_String8List *stream = (MD_String8List*)user;
    MD_S8ListPush(arena, stream, MD_S8((MD_u8*)str, size));
}

static inline MD_Node *
GetChildValue(MD_Node *parent, MD_String8 name)
{
    MD_Node *label_node = MD_ChildFromString(parent, name, 0);
    if (!label_node) return 0;

    if (label_node->first_child) return label_node->first_child;

    return label_node;
}

static inline MD_String8
GetChildString(MD_Node *parent, MD_String8 name)
{
    MD_Node *node = GetChildValue(parent, name);
    return node ? node->string : MD_S8Lit("");
}

static inline void
EvalTemplateNode(MD_Arena *arena, MD_Node *node, MD_String8List *out,
        MD_String8 title, MD_String8 description, MD_String8 content,
        MD_String8 date)
{
    for (MD_EachNode(it, node->first_child))
    {
        if (it->flags & MD_NodeFlag_StringLiteral)
        {
            MD_S8ListPush(arena, out, it->string);
        }
        else if(it->flags & MD_NodeFlag_Identifier)
        {
            if (0){}
            else if (MD_S8Match(it->string, MD_S8Lit("title"), 0))
            {
                MD_S8ListPush(arena, out, title);
            }
            else if (MD_S8Match(it->string, MD_S8Lit("description"), 0))
            {
                MD_S8ListPush(arena, out, description);
            }
            else if (MD_S8Match(it->string, MD_S8Lit("content"), 0))
            {
                MD_S8ListPush(arena, out, content);
            }
            else if (MD_S8Match(it->string, MD_S8Lit("date"), 0))
            {
                MD_S8ListPush(arena, out, date);
            }
            else
            {
                MD_CodeLoc code_loc = MD_CodeLocFromNode(it);
                MD_PrintMessage(stderr, code_loc, MD_MessageKind_Error,
                        MD_S8Fmt(arena, "Unknown variable \"%.*s\"", MD_S8VArg(it->string)));
            }
        }
    }
}

int main(int argc, char **argv)
{
    arena = MD_ArenaAlloc();

    MD_ParseResult parse = MD_ParseWholeFile(arena, MD_S8Lit("posts.mdesk"));

    for (MD_Message *message = parse.errors.first;
            message != 0;
            message = message->next)
    {
        MD_CodeLoc code_loc = MD_CodeLocFromNode(message->node);
        MD_PrintMessage(stdout, code_loc, message->kind, message->string);
    }

    if (parse.errors.max_message_kind >= MD_MessageKind_Error)
    {
        return 1;
    }

    for (MD_EachNode(node, parse.node->first_child))
    {
        if (MD_NodeHasTag(node, MD_S8Lit("post"), 0))
        {
            MD_String8 title = GetChildString(node, MD_S8Lit("title"));
            MD_String8 description = GetChildString(node, MD_S8Lit("description"));
            MD_String8 filename = GetChildString(node, MD_S8Lit("file"));
            MD_String8 date = GetChildString(node, MD_S8Lit("date"));

            if (filename.size)
            {
                MD_String8 file_contents = MD_LoadEntireFile(arena, filename);
                MD_String8List stream = {0};
                md_html((MD_CHAR*)file_contents.str, file_contents.size,
                        ProcessHTMLChunk,
                        &stream,
                        MD_FLAG_TABLES|MD_FLAG_STRIKETHROUGH|MD_FLAG_COLLAPSEWHITESPACE,
                        0);
                MD_String8 content = MD_S8ListJoin(arena, stream, 0);

                MD_Node *template = MD_ChildFromString(parse.node, MD_S8Lit("template"), 0);
                MD_String8List out = {0};
                EvalTemplateNode(arena, template, &out, title, description, content, date);
                MD_String8 str = MD_S8ListJoin(arena, out, 0);

                MD_String8 short_name = MD_PathChopLastPeriod(MD_PathSkipLastSlash(filename));
                MD_SaveEntireFile(arena, str, MD_S8Fmt(arena, "public/%.*s.html", MD_S8VArg(short_name)));
            }
        }
    }

    return 0;
}
