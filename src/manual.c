/* 
   Copyright (C) Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License  
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of Cfengine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.

*/

/*****************************************************************************/
/*                                                                           */
/* File: manual.c                                                            */
/*                                                                           */
/*****************************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"
#include "manual.h"
#include "writer.h"

extern char BUILD_DIR[CF_BUFSIZE];
extern char MANDIR[CF_BUFSIZE];

static void TexinfoHeader(FILE *fout);
static void TexinfoFooter(FILE *fout);
static void TexinfoBodyParts(FILE *fout, const BodySyntax *bs, char *context);
static void TexinfoSubBodyParts(FILE *fout, BodySyntax *bs);
static void TexinfoShowRange(FILE *fout, char *s, enum cfdatatype type);
static void IncludeManualFile(FILE *fout, char *filename);
static void TexinfoPromiseTypesFor(FILE *fout, SubTypeSyntax *st);
static void TexinfoSpecialFunction(FILE *fout, FnCallType fn);
static void TexinfoVariables(FILE *fout, char *scope);
static char *TexInfoEscape(char *s);
static void PrintPattern(FILE *fout, const char *pattern);

/*****************************************************************************/

void TexinfoManual(char *mandir)
{
    char filename[CF_BUFSIZE];
    SubTypeSyntax *st;
    Item *done = NULL;
    FILE *fout;
    int i;

    snprintf(filename, CF_BUFSIZE - 1, "%scf3-Reference.texinfo", BUILD_DIR);

    if ((fout = fopen(filename, "w")) == NULL)
    {
        CfOut(cf_error, "fopen", "Unable to open %s for writing\n", filename);
        return;
    }

    TexinfoHeader(fout);

/* General background */

    fprintf(fout, "@c *****************************************************\n");
    fprintf(fout, "@c * CHAPTER \n");
    fprintf(fout, "@c *****************************************************\n");

    fprintf(fout, "@node Getting started\n@chapter CFEngine %s -- Getting started\n\n", Version());
    IncludeManualFile(fout, "reference_basics.texinfo");

/* Control promises */

    fprintf(fout, "@c *****************************************************\n");
    fprintf(fout, "@c * CHAPTER \n");
    fprintf(fout, "@c *****************************************************\n");

    fprintf(fout, "@node Control Promises\n@chapter Control promises\n\n");
    IncludeManualFile(fout, "reference_control_intro.texinfo");

    fprintf(fout, "@menu\n");
    for (i = 0; CF_ALL_BODIES[i].btype != NULL; ++i)
    {
        fprintf(fout, "* control %s::\n", CF_ALL_BODIES[i].btype);
    }
    fprintf(fout, "@end menu\n");

    for (i = 0; CF_ALL_BODIES[i].btype != NULL; i++)
    {
        fprintf(fout, "@node control %s\n@section @code{%s} control promises\n\n", CF_ALL_BODIES[i].btype,
                CF_ALL_BODIES[i].btype);
        snprintf(filename, CF_BUFSIZE - 1, "control/%s_example.texinfo", CF_ALL_BODIES[i].btype);
        IncludeManualFile(fout, filename);
        snprintf(filename, CF_BUFSIZE - 1, "control/%s_notes.texinfo", CF_ALL_BODIES[i].btype);
        IncludeManualFile(fout, filename);

        TexinfoBodyParts(fout, CF_ALL_BODIES[i].bs, CF_ALL_BODIES[i].btype);
    }

/* Components */

    for (i = 0; i < CF3_MODULES; i++)
    {
        st = (CF_ALL_SUBTYPES[i]);

        if (st == CF_COMMON_SUBTYPES || st == CF_EXEC_SUBTYPES || st == CF_REMACCESS_SUBTYPES
            || st == CF_KNOWLEDGE_SUBTYPES || st == CF_MEASUREMENT_SUBTYPES)

        {
            CfOut(cf_verbose, "", "Dealing with chapter / bundle type %s\n", st->btype);
            fprintf(fout, "@c *****************************************************\n");
            fprintf(fout, "@c * CHAPTER \n");
            fprintf(fout, "@c *****************************************************\n");

            if (strcmp(st->btype, "*") == 0)
            {
                fprintf(fout, "@node Bundles for common\n@chapter Bundles of @code{common}\n\n");
            }
            else
            {
                fprintf(fout, "@node Bundles for %s\n@chapter Bundles of @code{%s}\n\n", st->btype, st->btype);
            }
        }

        if (!IsItemIn(done, st->btype)) /* Avoid multiple reading if several modules */
        {
            PrependItem(&done, st->btype, NULL);
            snprintf(filename, CF_BUFSIZE - 1, "bundletypes/%s_example.texinfo", st->btype);
            IncludeManualFile(fout, filename);
            snprintf(filename, CF_BUFSIZE - 1, "bundletypes/%s_notes.texinfo", st->btype);
            IncludeManualFile(fout, filename);

            fprintf(fout, "@menu\n");
            for (int k = 0; k < CF3_MODULES; ++k)
            {
                for (int j = 0; CF_ALL_SUBTYPES[k][j].btype != NULL; ++j)
                {
                    fprintf(fout, "* %s in %s promises::\n", CF_ALL_SUBTYPES[k][j].subtype,
                            strcmp(CF_ALL_SUBTYPES[k][j].btype, "*") == 0 ? "common" : CF_ALL_SUBTYPES[k][j].btype);
                }
            }
            fprintf(fout, "@end menu\n");
        }

        TexinfoPromiseTypesFor(fout, st);
    }

/* Special functions */

    CfOut(cf_verbose, "", "Dealing with chapter / bundle type - special functions\n");
    fprintf(fout, "@c *****************************************************\n");
    fprintf(fout, "@c * CHAPTER \n");
    fprintf(fout, "@c *****************************************************\n");

    fprintf(fout, "@node Special functions\n@chapter Special functions\n\n");

    fprintf(fout, "@menu\n");
    fprintf(fout, "* Introduction to functions::\n");

    for (i = 0; CF_FNCALL_TYPES[i].name != NULL; ++i)
    {
        fprintf(fout, "* Function %s::\n", CF_FNCALL_TYPES[i].name);
    }

    fprintf(fout, "@end menu\n");

    fprintf(fout, "@node Introduction to functions\n@section Introduction to functions\n\n");

    IncludeManualFile(fout, "functions_intro.texinfo");

    for (i = 0; CF_FNCALL_TYPES[i].name != NULL; i++)
    {
        fprintf(fout, "@node Function %s\n@section Function %s \n\n", CF_FNCALL_TYPES[i].name, CF_FNCALL_TYPES[i].name);
        TexinfoSpecialFunction(fout, CF_FNCALL_TYPES[i]);
    }

/* Special variables */

    CfOut(cf_verbose, "", "Dealing with chapter / bundle type - special variables\n");
    fprintf(fout, "@c *****************************************************\n");
    fprintf(fout, "@c * CHAPTER \n");
    fprintf(fout, "@c *****************************************************\n");

    fprintf(fout, "@node Special Variables\n@chapter Special Variables\n\n");

    static const char *scopes[] =
{
        "const",
        "edit",
        "match",
        "mon",
        "sys",
        "this",
        NULL,
    };

    fprintf(fout, "@menu\n");
    for (const char **s = scopes; *s != NULL; ++s)
    {
        fprintf(fout, "* Variable context %s::\n", *s);
    }
    fprintf(fout, "@end menu\n");

// scopes const and sys

    NewScope("edit");
    NewScalar("edit", "filename", "x", cf_str);

    for (const char **s = scopes; *s != NULL; ++s)
    {
        TexinfoVariables(fout, (char *) *s);
    }

// Log files

    CfOut(cf_verbose, "", "Dealing with chapter / bundle type - Logs and records\n");
    fprintf(fout, "@c *****************************************************\n");
    fprintf(fout, "@c * CHAPTER \n");
    fprintf(fout, "@c *****************************************************\n");

    fprintf(fout, "@node Logs and records\n@chapter Logs and records\n\n");
    IncludeManualFile(fout, "reference_logs.texinfo");

    TexinfoFooter(fout);

    fclose(fout);
}

/*****************************************************************************/
/* Level                                                                     */
/*****************************************************************************/

static void TexinfoHeader(FILE *fout)
{
    fprintf(fout,
            "\\input texinfo-altfont\n"
            "\\input texinfo-logo\n"
            "\\input texinfo\n"
            "@selectaltfont{cmbright}\n"
            "@setlogo{CFEngineFrontPage}\n"
            "@c *********************************************************************\n"
            "@c\n"
            "@c  This is an AUTO_GENERATED TEXINFO file. Do not submit patches against it.\n"
            "@c  Refer to the the component .texinfo files instead when patching docs.\n"
            "@c\n"
            "@c ***********************************************************************\n"
            "@c %%** start of header\n"
            "@setfilename cf3-reference.info\n"
            "@settitle CFEngine reference manual\n"
            "@setchapternewpage odd\n"
            "@c %%** end of header\n"
            "@titlepage\n"
            "@title CFEngine Reference Manual\n" "@subtitle Auto generated, self-healing knowledge\n" "@subtitle %s\n"
#ifdef HAVE_NOVA
            "@subtitle %s\n"
#endif
#ifdef HAVE_CONSTELLATION
            "@subtitle %s\n"
#endif
            "@author cfengine.com\n"
            "@c @smallbook\n"
            "@fonttextsize 10\n"
            "@page\n"
            "@vskip 0pt plus 1filll\n"
            "@cartouche\n"
            "Under no circumstances shall CFEngine AS be liable for errors or omissions\n"
            "in this document. All efforts have been made to ensure the correctness of\n"
            "the information contained herein.\n"
            "@end cartouche\n"
            "Copyright @copyright{} 2008,2010 to the year of issue CFEngine AS\n"
            "@end titlepage\n"
            "@c *************************** File begins here ************************\n"
            "@ifinfo\n"
            "@dircategory CFEngine Training\n"
            "@direntry\n"
            "* cfengine Reference:\n"
            "                        CFEngine is a language based framework\n"
            "                        designed for configuring and maintaining\n"
            "                        Unix-like operating systems attached\n"
            "                        to a TCP/IP network.\n"
            "@end direntry\n"
            "@end ifinfo\n"
            "@ifnottex\n"
            "@node Top\n"
            "@top CFEngine-AutoReference\n"
            "@end ifnottex\n"
            "@menu\n"
            "* Getting started::\n"
            "* A simple crash course::\n"
            "* How to run CFEngine 3 examples::\n"
            "* A complete configuration::\n"
            "* Control Promises::\n"
            "* Bundles for common::\n"
            "* Bundles for agent::\n"
            "* Bundles for server::\n"
            "* Bundles for knowledge::\n"
            "* Bundles for monitor::\n"
            "* Special functions::\n"
            "* Special Variables::\n"
            "* Logs and records::\n"
            "@end menu\n"
            "@ifhtml\n"
            "@html\n"
            "<a href=\"#Contents\"><h1>COMPLETE TABLE OF CONTENTS</h1></a>\n"
            "<h2>Summary of contents</h2>\n"
            "@end html\n" "@end ifhtml\n" "@iftex\n" "@contents\n" "@end iftex\n", NameVersion()
#ifdef HAVE_NOVA
            , Nova_NameVersion()
#endif
#ifdef HAVE_CONSTELLATION
            , Constellation_NameVersion()
#endif
        );
}

/*****************************************************************************/

static void TexinfoFooter(FILE *fout)
{
    fprintf(fout,
            "@c =========================================================================\n"
            "@c @node Index\n"
            "@c @unnumbered Concept Index\n"
            "@c @printindex cp\n"
            "@c =========================================================================\n"
            "@ifhtml\n"
            "@html\n"
            "<a name=\"Contents\">\n"
            "@contents\n"
            "@end html\n"
            "@end ifhtml\n"
            "@c  The file is structured like a programming language. Each chapter\n"
            "@c  starts with a chapter comment.\n"
            "@c\n"
            "@c  Menus list the subsections so that an online info-reader can parse\n"
            "@c  the file hierarchically.\n"
            "@ifhtml\n"
            "@html\n"
            "<script type=\"text/javascript\">\n"
            "var gaJsHost = ((\"https:\" == document.location.protocol) ? \"https://ssl.\" : \"http://www.\");\n"
            "document.write(unescape(\"%%3Cscript src='\" + gaJsHost + \"google-analytics.com/ga.js' type='text/javascript'%%3E%%3C/script%%3E\"));\n"
            "</script>\n"
            "<script type=\"text/javascript\">\n"
            "var pageTracker = _gat._getTracker(\"UA-2576171-2\");\n"
            "pageTracker._initData();\n"
            "pageTracker._trackPageview();\n" "</script>\n" "@end html\n" "@end ifhtml\n" "@bye\n");
}

/*****************************************************************************/

static void TexinfoPromiseTypesFor(FILE *fout, SubTypeSyntax *st)
{
    int j;
    char filename[CF_BUFSIZE];

/* Each array element is SubtypeSyntax representing an agent-promise assoc */

    for (j = 0; st[j].btype != NULL; j++)
    {
        CfOut(cf_verbose, "", " - Dealing with promise type %s\n", st[j].subtype);

        if (strcmp("*", st[j].btype) == 0)
        {
            fprintf(fout, "\n\n@node %s in common promises\n@section @code{%s} promises\n\n", st[j].subtype,
                    st[j].subtype);
            snprintf(filename, CF_BUFSIZE - 1, "promise_common_intro.texinfo");
        }
        else
        {
            fprintf(fout, "\n\n@node %s in %s promises\n@section @code{%s} promises in @samp{%s}\n\n", st[j].subtype,
                    st[j].btype, st[j].subtype, st[j].btype);
            snprintf(filename, CF_BUFSIZE - 1, "promises/%s_intro.texinfo", st[j].subtype);
            IncludeManualFile(fout, filename);
            snprintf(filename, CF_BUFSIZE - 1, "promises/%s_example.texinfo", st[j].subtype);
            IncludeManualFile(fout, filename);
            snprintf(filename, CF_BUFSIZE - 1, "promises/%s_notes.texinfo", st[j].subtype);
        }
        IncludeManualFile(fout, filename);
        TexinfoBodyParts(fout, st[j].bs, st[j].subtype);
    }
}

/*****************************************************************************/
/* Level                                                                     */
/*****************************************************************************/

static void TexinfoBodyParts(FILE *fout, const BodySyntax *bs, char *context)
{
    int i;
    char filename[CF_BUFSIZE];

    if (bs == NULL)
    {
        return;
    }

    fprintf(fout, "@menu\n");

    for (i = 0; bs[i].lval != NULL; ++i)
    {
        fprintf(fout, "* %s in %s::\n", bs[i].lval, context);
    }

    fprintf(fout, "@end menu\n");

    for (i = 0; bs[i].lval != NULL; i++)
    {
        CfOut(cf_verbose, "", " - -  Dealing with body type %s\n", bs[i].lval);

        if (bs[i].range == (void *) CF_BUNDLE)
        {
            fprintf(fout, "\n\n@node %s in %s\n@subsection @code{%s}\n\n@b{Type}: %s (Separate Bundle) \n", bs[i].lval,
                    context, bs[i].lval, CF_DATATYPES[bs[i].dtype]);
        }
        else if (bs[i].dtype == cf_body)
        {
            fprintf(fout, "\n\n@node %s in %s\n@subsection @code{%s} (body template)\n@noindent @b{Type}: %s\n\n",
                    bs[i].lval, context, bs[i].lval, CF_DATATYPES[bs[i].dtype]);
            TexinfoSubBodyParts(fout, (BodySyntax *) bs[i].range);
        }
        else
        {
            const char *res = bs[i].default_value;

            fprintf(fout, "\n\n@node %s in %s\n@subsection @code{%s}\n@noindent @b{Type}: %s\n\n", bs[i].lval, context,
                    bs[i].lval, CF_DATATYPES[bs[i].dtype]);
            TexinfoShowRange(fout, (char *) bs[i].range, bs[i].dtype);

            if (res)
            {
                fprintf(fout, "@noindent @b{Default value:} %s\n", res);
            }

            fprintf(fout, "\n@noindent @b{Synopsis}: %s\n\n", bs[i].description);
            fprintf(fout, "\n@noindent @b{Example}:@*\n");
            snprintf(filename, CF_BUFSIZE - 1, "bodyparts/%s_example.texinfo", bs[i].lval);
            IncludeManualFile(fout, filename);
            fprintf(fout, "\n@noindent @b{Notes}:@*\n");
            snprintf(filename, CF_BUFSIZE - 1, "bodyparts/%s_notes.texinfo", bs[i].lval);
            IncludeManualFile(fout, filename);
        }
    }
}

/*******************************************************************/

static void TexinfoVariables(FILE *fout, char *scope)
{
    char filename[CF_BUFSIZE], varname[CF_BUFSIZE];
    Rlist *rp, *list = NULL;
    int i;

    HashToList(GetScope(scope), &list);
    list = AlphaSortRListNames(list);

    fprintf(fout, "\n\n@node Variable context %s\n@section Variable context @code{%s}\n\n", scope, scope);
    snprintf(filename, CF_BUFSIZE - 1, "varcontexts/%s_intro.texinfo", scope);
    IncludeManualFile(fout, filename);

    fprintf(fout, "@menu\n");

    if (strcmp(scope, "mon") != 0)
    {
        for (rp = list; rp != NULL; rp = rp->next)
        {
            fprintf(fout, "* Variable %s.%s::\n", scope, (char *) rp->item);
        }
    }
    else
    {
        for (i = 0; i < CF_OBSERVABLES; ++i)
        {
            if (strcmp(OBS[i][0], "spare") == 0)
            {
                break;
            }

            fprintf(fout, "* Variable mon.value_%s::\n", OBS[i][0]);
            fprintf(fout, "* Variable mon.av_%s::\n", OBS[i][0]);
            fprintf(fout, "* Variable mon.dev_%s::\n", OBS[i][0]);
        }
    }

    fprintf(fout, "@end menu\n");

    for (rp = list; rp != NULL; rp = rp->next)
    {
        fprintf(fout, "@node Variable %s.%s\n@subsection Variable %s.%s \n\n", scope, (char *) rp->item, scope,
                (char *) rp->item);
        snprintf(filename, CF_BUFSIZE - 1, "vars/%s_%s.texinfo", scope, (char *) rp->item);
        IncludeManualFile(fout, filename);
    }

    DeleteRlist(list);

    if (strcmp(scope, "mon") == 0)
    {
        for (i = 0; i < CF_OBSERVABLES; i++)
        {
            if (strcmp(OBS[i][0], "spare") == 0)
            {
                break;
            }

            snprintf(varname, CF_MAXVARSIZE, "value_%s", OBS[i][0]);
            fprintf(fout, "\n@node Variable %s.%s\n@subsection Variable %s.%s \n\n", scope, varname, scope, varname);
            fprintf(fout, "Observational measure collected every 2.5 minutes from cf-monitord, description: @var{%s}.",
                    OBS[i][1]);

            snprintf(varname, CF_MAXVARSIZE, "av_%s", OBS[i][0]);
            fprintf(fout, "\n@node Variable %s.%s\n@subsection Variable %s.%s \n\n", scope, varname, scope, varname);
            fprintf(fout, "Observational measure collected every 2.5 minutes from cf-monitord, description: @var{%s}.",
                    OBS[i][1]);

            snprintf(varname, CF_MAXVARSIZE, "dev_%s", OBS[i][0]);
            fprintf(fout, "\n@node Variable %s.%s\n@subsection Variable %s.%s \n\n", scope, varname, scope, varname);
            fprintf(fout, "Observational measure collected every 2.5 minutes from cf-monitord, description: @var{%s}.",
                    OBS[i][1]);
        }
    }

}

/*******************************************************************/
/* Level                                                           */
/*******************************************************************/

static void TexinfoShowRange(FILE *fout, char *s, enum cfdatatype type)
{
    Rlist *list = NULL, *rp;

    if (strlen(s) == 0)
    {
        fprintf(fout, "@noindent @b{Allowed input range}: (arbitrary string)\n\n");
        return;
    }

    if (type == cf_opts || type == cf_olist)
    {
        list = SplitStringAsRList(s, ',');
        fprintf(fout, "@noindent @b{Allowed input range}: @*\n@example");

        for (rp = list; rp != NULL; rp = rp->next)
        {
            fprintf(fout, "\n          @code{%s}", (char *) rp->item);
        }

        fprintf(fout, "\n@end example\n");
        DeleteRlist(list);
    }
    else
    {
        fprintf(fout, "@noindent @b{Allowed input range}: @code{%s}\n\n", TexInfoEscape(s));
    }
}

/*****************************************************************************/

static void TexinfoSubBodyParts(FILE *fout, BodySyntax *bs)
{
    int i;
    char filename[CF_BUFSIZE];

    if (bs == NULL)
    {
        return;
    }

    fprintf(fout, "@table @samp\n");

    for (i = 0; bs[i].lval != NULL; i++)
    {
        if (bs[i].range == (void *) CF_BUNDLE)
        {
            fprintf(fout, "@item @code{%s}\n@b{Type}: %s\n (Separate Bundle) \n\n", bs[i].lval,
                    CF_DATATYPES[bs[i].dtype]);
        }
        else if (bs[i].dtype == cf_body)
        {
            fprintf(fout, "@item @code{%s}\n@b{Type}: %s\n\n", bs[i].lval, CF_DATATYPES[bs[i].dtype]);
            TexinfoSubBodyParts(fout, (BodySyntax *) bs[i].range);
        }
        else
        {
            const char *res = bs[i].default_value;

            fprintf(fout, "@item @code{%s}\n@b{Type}: %s\n\n", bs[i].lval, CF_DATATYPES[bs[i].dtype]);
            TexinfoShowRange(fout, (char *) bs[i].range, bs[i].dtype);
            fprintf(fout, "\n@noindent @b{Synopsis}: %s\n\n", bs[i].description);

            if (res)
            {
                fprintf(fout, "\n@noindent @b{Default value:} %s\n", res);
            }

            fprintf(fout, "\n@b{Example}:@*\n");
            snprintf(filename, CF_BUFSIZE - 1, "bodyparts/%s_example.texinfo", bs[i].lval);
            IncludeManualFile(fout, filename);
            fprintf(fout, "\n@b{Notes}:@*\n");
            snprintf(filename, CF_BUFSIZE - 1, "bodyparts/%s_notes.texinfo", bs[i].lval);
            IncludeManualFile(fout, filename);
        }
    }

    fprintf(fout, "@end table\n");
}

/*****************************************************************************/

static bool GenerateStub(const char *filename)
{
    FILE *fp;

    if ((fp = fopen(filename, "w")) == NULL)
    {
        CfOut(cf_inform, "fopen", "Could not write to manual source %s\n", filename);
        return false;
    }

#ifdef HAVE_CONSTELLATION
    fprintf(fp, "\n@i{History}: Was introduced in %s, Nova %s, Constellation %s (%d)\n\n",
            Version(), Nova_Version(), Constellation_Version(), BUILD_YEAR);
#elif HAVE_NOVA
    fprintf(fp, "\n@i{History}: Was introduced in %s, Nova %s (%d)\n\n", Version(), Nova_Version(), BUILD_YEAR);
#else
    fprintf(fp, "\n@i{History}: Was introduced in %s (%d)\n\n", Version(), BUILD_YEAR);
#endif
    fprintf(fp, "\n@verbatim\n\nFill me in (%s)\n\"\"\n@end verbatim\n", filename);
    fclose(fp);
    CfOut(cf_verbose, "", "Created %s template\n", filename);
    return true;
}

/*****************************************************************************/

char *ReadTexinfoFileF(const char *fmt, ...)
{
    Writer *filenamew = StringWriter();

    struct stat sb;
    char *buffer = NULL;
    FILE *fp = NULL;
    off_t file_size;

    va_list ap;

    va_start(ap, fmt);
    WriterWriteF(filenamew, "%s/", MANDIR);
    WriterWriteVF(filenamew, fmt, ap);
    va_end(ap);

    char *filename = StringWriterClose(filenamew);

    if (cfstat(filename, &sb) == -1)
    {
        if (!GenerateStub(filename))
        {
            CfOut(cf_inform, "", "Unable to write down stub for missing texinfo file");
            free(filename);
            return NULL;
        }
    }

    if ((fp = fopen(filename, "r")) == NULL)
    {
        CfOut(cf_inform, "fopen", "Could not read manual source %s\n", filename);
        free(filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftello(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = (char *) xcalloc(file_size + 1, sizeof(char));
    buffer[file_size] = '\0';
    int cnt = fread(buffer, sizeof(char), file_size, fp);

    if (ferror(fp) || cnt != file_size)
    {
        CfOut(cf_inform, "fread", "Could not read manual source %s\n", filename);
        free(buffer);
        fclose(fp);
        free(filename);
        return NULL;
    }

    fclose(fp);
    free(filename);

    return buffer;
}

/*****************************************************************************/

static void IncludeManualFile(FILE *fout, char *file)
{
    char *contents = ReadTexinfoFileF("%s", file);

    if (contents)
    {
        fprintf(fout, "@*\n%s\n", contents);
    }
}

/*****************************************************************************/

static void TexinfoSpecialFunction(FILE *fout, FnCallType fn)
{
    char filename[CF_BUFSIZE];
    const FnCallArg *args = fn.args;
    int i;

    fprintf(fout, "\n@noindent @b{Synopsis}: %s(", fn.name);

    for (i = 0; args[i].pattern != NULL; i++)
    {
        fprintf(fout, "arg%d", i + 1);

        if (args[i + 1].pattern != NULL)
        {
            fprintf(fout, ",");
        }
    }
    if (fn.varargs)
    {
        if (i != 0)
        {
            fprintf(fout, ",");
        }
        fprintf(fout, "...");
    }

    fprintf(fout, ") returns type @b{%s}\n\n@*\n", CF_DATATYPES[fn.dtype]);

    for (i = 0; args[i].pattern != NULL; i++)
    {
        fprintf(fout, "@noindent @code{  } @i{arg%d} : %s, @i{in the range} ", i + 1, args[i].description);
        PrintPattern(fout, args[i].pattern);
        fprintf(fout, "\n@*\n");
    }

    fprintf(fout, "\n@noindent %s\n\n", fn.description);
    fprintf(fout, "\n@noindent @b{Example}:@*\n");

    snprintf(filename, CF_BUFSIZE - 1, "functions/%s_example.texinfo", fn.name);
    IncludeManualFile(fout, filename);

    fprintf(fout, "\n@noindent @b{Notes}:@*\n");
    snprintf(filename, CF_BUFSIZE - 1, "functions/%s_notes.texinfo", fn.name);
    IncludeManualFile(fout, filename);
}

/*****************************************************************************/

static void PrintPattern(FILE *fout, const char *pattern)
{
    const char *sp;

    for (sp = pattern; *sp != '\0'; sp++)
    {
        switch (*sp)
        {
        case '@':
        case '{':
        case '}':
            fputc((int) '@', fout);
        default:
            fputc((int) *sp, fout);
        }
    }
}

/*****************************************************************************/

char *TexInfoEscape(char *s)
{
    char *spf, *spt;
    static char buffer[CF_BUFSIZE];

    memset(buffer, 0, CF_BUFSIZE);

    for (spf = s, spt = buffer; *spf != '\0'; spf++)
    {
        switch (*spf)
        {
        case '{':
        case '}':
        case '@':
            *spt++ = '@';
            break;
        }

        *spt++ = *spf;
    }

    return buffer;
}
