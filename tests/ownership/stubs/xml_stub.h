/* Synthetic libxml2/libxslt/libexslt boundary for the ownership harness.
   Only the symbols 4DPlugin-XSLT.cpp uses are declared; the implementation
   lives in fake4d.cpp and tracks every allocation. */
#ifndef XSLT_OWNERSHIP_XML_STUB_H
#define XSLT_OWNERSHIP_XML_STUB_H

#include <stddef.h>
#include <stdio.h>

typedef unsigned char xmlChar;
typedef struct StubDoc *xmlDocPtr;
typedef struct StubStylesheet *xsltStylesheetPtr;
typedef struct StubTransformContext *xsltTransformContextPtr;
typedef struct StubOutputBuffer *xmlOutputBufferPtr;
typedef void *xmlCharEncodingHandlerPtr;

#define XSLT_PARSE_OPTIONS 16398

void xmlInitParser(void);
void xmlCleanupParser(void);
void exsltRegisterAll(void);
void xsltCleanupGlobals(void);

xmlDocPtr xmlParseFile(const char *filename);
xmlDocPtr xmlParseMemory(const char *buffer, int size);
int xmlXIncludeProcessFlags(xmlDocPtr doc, int flags);
void xmlFreeDoc(xmlDocPtr doc);

xsltStylesheetPtr xsltParseStylesheetDoc(xmlDocPtr doc);
void xsltFreeStylesheet(xsltStylesheetPtr style);
xsltTransformContextPtr xsltNewTransformContext(xsltStylesheetPtr style, xmlDocPtr doc);
void xsltFreeTransformContext(xsltTransformContextPtr ctxt);
xmlDocPtr xsltApplyStylesheetUser(xsltStylesheetPtr style, xmlDocPtr doc, const char **params,
                                  const char *output, FILE *profile, xsltTransformContextPtr userCtxt);

xmlOutputBufferPtr xmlAllocOutputBuffer(xmlCharEncodingHandlerPtr encoder);
int xsltSaveResultTo(xmlOutputBufferPtr buf, xmlDocPtr result, xsltStylesheetPtr style);
size_t xmlOutputBufferGetSize(xmlOutputBufferPtr out);
const xmlChar *xmlOutputBufferGetContent(xmlOutputBufferPtr out);
int xmlOutputBufferClose(xmlOutputBufferPtr out);

#endif
