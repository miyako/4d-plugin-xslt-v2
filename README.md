![version](https://img.shields.io/badge/version-17%2B-3E8B93)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-xslt-v2)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-xslt-v2/total)

# 4d-plugin-xslt-v2

The XSLT plugin applies an XSLT 1.0 stylesheet to an XML document, using libxml2/libxslt (the same C libraries most XML tooling is built on) with EXSLT extensions registered automatically at plugin load — so stylesheets can use extension namespaces like `http://exslt.org/strings` (`str:`) in addition to plain XSLT 1.0. It exposes a single command, `XSLT Apply stylesheet`, which takes an XML document and an XSL stylesheet (each as a `Blob`, either the document's raw bytes or a path to a file on disk) plus an optional options object, and returns the transformed result as a `Blob`.

## Summary

| Command | Returns | Purpose |
|---|---|---|
| [XSLT Apply stylesheet](#xslt-apply-stylesheet) | Blob | Apply an XSLT stylesheet to an XML document and return the transformed result. |

**Platforms:** macOS (Intel & Apple Silicon), Windows 64-bit.

---

## Requirements & platform notes

- Requires 4D v17 or later.
- `xml` and `xsl` are mandatory `Blob` parameters; `params` is optional.
- **Failure is silent, not a 4D error.** If the XML or XSL fails to parse, the transform fails, or there's simply nothing to write, the command returns an empty (0-byte) `Blob`. There is no error code or exception — always check the size of the returned blob before assuming success.
- Both `xml` and `xsl` accept two interchangeable forms:
  - The actual document bytes (e.g. from `DOCUMENT TO BLOB` or `CONVERT FROM TEXT`).
  - A blob whose content is a file path string, in which case the plugin reads that file from disk instead of parsing the blob's bytes directly. This only kicks in when the blob is under 1024 bytes *and* its content passes 4D's internal path-validity check — an ordinary XML/XSL blob under 1024 bytes is exceedingly unlikely to also look like a valid path, but keep the size threshold in mind if you're feeding in very small synthetic documents.
  - **On macOS**, a path-form blob is expected to be a classic HFS colon-separated path; the plugin converts it to a POSIX path internally (via `CFURLCreateWithFileSystemPath`) before handing it to libxml2. A path built in modern POSIX slash form (e.g. straight from `Get 4D folder`) may not resolve correctly through that conversion — if in doubt, pass the document's raw bytes instead of a path.
  - **On Windows**, a path-form blob is used as-is, with no path-style conversion.
- XInclude processing (`xmlXIncludeProcessFlags`) is applied to both the parsed XML document and the parsed XSL stylesheet, using `xmlParserOption`/`xslParserOption` respectively as the option flags.
- Stylesheet parameter values (any string-valued property in `params` other than the two reserved names below) are XPath expressions, not raw literal text. To pass a literal string, quote it yourself, e.g. `"'stroke'"` — an unquoted value like `"stroke"` is evaluated as an XPath expression (e.g. a variable/node reference), not treated as the literal text `stroke`.
- Only **string-valued** properties in `params` are passed through as stylesheet parameters; numbers, booleans, objects, and arrays are silently ignored for that purpose.
- The two reserved property names, `xmlParserOption` and `xslParserOption`, must be numbers. Because the same property scan treats any string-valued property as a stylesheet parameter regardless of its name, accidentally passing a string for one of these reserved names both fails to set the parser option (silently — a non-numeric value is simply not applied) and adds an unintended stylesheet parameter of that name.

---

## XSLT Apply stylesheet

### Syntax

```4d
XSLT Apply stylesheet ( xml ; xsl ; params ) → Blob
```

| Parameter | Type | Description |
|---|---|---|
| `xml` | Blob | XML source document — either its raw bytes, or (if under 1024 bytes and recognized as a valid path) a blob containing a file path to read from disk. |
| `xsl` | Blob | XSLT stylesheet, same two accepted forms as `xml`. |
| `params` | Object | Optional. May contain the reserved numeric properties `xmlParserOption`/`xslParserOption`, plus any number of string-valued properties that become XSLT stylesheet parameters. Omit, or pass an empty object, for defaults. |
| Result | Blob | The transformed output. Empty (0-byte) on any parse, transform, or output failure. |

### Description

`xml` and `xsl` are parsed with libxml2, then the stylesheet is applied with `xsltApplyStylesheetUser` and serialized back out with `xsltSaveResultTo`. The result's byte encoding and format (text/XML/HTML) follow whatever the stylesheet's own `<xsl:output>` declares (`method`, `encoding`) — libxslt's default output encoding when none is declared is UTF-8.

**Parser options.** Both `xml` and `xsl` are parsed with the same base set of libxml2 flags — `XML_PARSE_NOENT | XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR | XML_PARSE_NOCDATA` (libxslt's own recommended default set for loading XSLT-bound documents) — OR'd with whatever numeric value you supply in `params.xmlParserOption` (for the XML document) or `params.xslParserOption` (for the XSL stylesheet). Both accept the same set of libxml2 parser-option flags, exposed as 4D constants under the "XML Parse Options" theme:

| Constant | Value |
|---|---|
| `XML_PARSE_RECOVER` | 1 |
| `XML_PARSE_NOENT` | 2 |
| `XML_PARSE_DTDLOAD` | 4 |
| `XML_PARSE_DTDATTR` | 8 |
| `XML_PARSE_DTDVALID` | 16 |
| `XML_PARSE_NOERROR` | 32 |
| `XML_PARSE_NOWARNING` | 64 |
| `XML_PARSE_PEDANTIC` | 128 |
| `XML_PARSE_NOBLANKS` | 256 |
| `XML_PARSE_SAX1` | 512 |
| `XML_PARSE_XINCLUDE` | 1024 |
| `XML_PARSE_NONET` | 2048 |
| `XML_PARSE_NODICT` | 4096 |
| `XML_PARSE_NSCLEAN` | 8192 |
| `XML_PARSE_NOCDATA` | 16384 |
| `XML_PARSE_NOXINCNODE` | 32768 |
| `XML_PARSE_COMPACT` | 65536 |
| `XML_PARSE_OLD10` | 131072 |
| `XML_PARSE_NOBASEFIX` | 262144 |
| `XML_PARSE_HUGE` | 524288 |
| `XML_PARSE_OLDSAX` | 1048576 |
| `XML_PARSE_IGNORE_ENC` | 2097152 |
| `XML_PARSE_BIG_LINES` | 4194304 |

These match libxml2's own `xmlParserOption` enum bit-for-bit; check the libxml2 documentation if you need the exact per-flag semantics beyond the name.

### Example

From the plugin's own test method (`TEST.4dm`):

```4d
//%attributes = {}
$params:=New object:C1471
$params.str1:="'stroke'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.xsl"
$xmlPath:=Get 4D folder:C485(Current resources folder:K5:16)+"apple.svg"

DOCUMENT TO BLOB:C525($xmlPath;$xmlData)
DOCUMENT TO BLOB:C525($xslPath;$xslData)

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text:C1012($xsltData;"utf-8")

ALERT:C41("XSLT Apply stylesheet done")
```

Note the `str1` parameter value is written as `"'stroke'"` — a string containing single quotes — because the stylesheet's own `<xsl:param name="str1" .../>` is evaluated as an XPath expression; the extra quotes make it a literal string rather than an XPath reference to something named `stroke`.

From the plugin's README (an alternative, untagged form of the same idea, passing a path-form blob for `xml`/`xsl` instead of the document's raw bytes):

```4d
$params:=New object
$params.str1:="'test'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder(Current resources folder)+"sample.xsl"
$xmlPath:=Get 4D folder(Current resources folder)+"apple.svg"

CONVERT FROM TEXT($xmlPath;"utf-8";$xmlFile)
CONVERT FROM TEXT($xslPath;"utf-8";$xslFile)

$xsltData:=XSLT Apply stylesheet ($xmlFile;$xslFile;$params)
$xslt:=Convert to text($xsltData;"utf-8")
```

Here `$xmlFile`/`$xslFile` are blobs whose *content* is the path string itself (produced by `CONVERT FROM TEXT` on the path), which is exactly the path-form case described above — the plugin reads `$xmlPath`/`$xslPath` from disk rather than parsing the blob bytes as XML.

Overriding the default parser options and passing multiple stylesheet parameters:

```4d
$params:=New object:C1471
$params.xmlParserOption:=XML_PARSE_NOBLANKS
$params.str1:="'stroke'"
$params.mode:="'summary'"

$xslPath:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.xsl"
$xmlPath:=Get 4D folder:C485(Current resources folder:K5:16)+"apple.svg"

DOCUMENT TO BLOB:C525($xmlPath;$xmlData)
DOCUMENT TO BLOB:C525($xslPath;$xslData)

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text:C1012($xsltData;"utf-8")

If ($xslt="")
  ALERT:C41("Transform failed or produced no output")
End if
```

---

## Error handling & troubleshooting

- **An empty result doesn't mean an error occurred — it's the only failure signal you get.** Bad XML, a bad stylesheet, a failed transform, or a stylesheet that genuinely produces no output all look identical: a 0-byte `Blob`, with no 4D error raised. Check the returned blob's size before trusting the result.
- **A path-form `xml`/`xsl` blob only works under 1024 bytes.** Larger blobs are always parsed as document content, never as a path, regardless of what they contain.
- **On macOS, path-form blobs must be HFS-style (colon-separated), not POSIX-style.** A POSIX-style path (forward slashes) run through the plugin's internal HFS→POSIX conversion may not resolve to the file you expect. Prefer passing raw document bytes over a path-form blob on macOS if you're unsure of the path format.
- **An unquoted stylesheet parameter is an XPath expression, not a literal string.** If a parameter value doesn't render the way you expect, check whether it needs to be wrapped in its own quotes (`"'like this'"`).
- **`xmlParserOption`/`xslParserOption` must be numbers.** A string value there is silently ignored as an option and instead passed to the stylesheet as a same-named parameter — a config typo can quietly turn into a stray stylesheet parameter.
- **Non-string properties in `params` never become stylesheet parameters.** If a parameter isn't reaching your stylesheet, confirm its value in `params` is a string (quoted appropriately per the point above), not a number, boolean, object, or array.

---

## Quick reference

```4d
$params:=New object:C1471
$params.xmlParserOption:=XML_PARSE_NOBLANKS
$params.myParam:="'literal value'"

$xmlPath:=Get 4D folder:C485(Current resources folder:K5:16)+"input.xml"
$xslPath:=Get 4D folder:C485(Current resources folder:K5:16)+"stylesheet.xsl"

DOCUMENT TO BLOB:C525($xmlPath;$xmlData)
DOCUMENT TO BLOB:C525($xslPath;$xslData)

$result:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text:C1012($result;"utf-8")

If ($xslt="")
  ALERT:C41("Transform failed or produced no output")
End if
```
