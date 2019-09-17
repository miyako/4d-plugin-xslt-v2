//%attributes = {}
$params:=New object:C1471
$params.str1:="'test'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.xsl"
$xmlPath:=Get 4D folder:C485(Current resources folder:K5:16)+"apple.svg"

DOCUMENT TO BLOB:C525($xmlPath;$xmlData)
DOCUMENT TO BLOB:C525($xslPath;$xslData)

CONVERT FROM TEXT:C1011($xmlPath;"utf-8";$xmlFile)
CONVERT FROM TEXT:C1011($xslPath;"utf-8";$xslFile)

$xsltData:=XSLT Apply stylesheet ($xmlFile;$xslFile;$params)
$xslt:=Convert to text:C1012($xsltData;"utf-8")

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text:C1012($xsltData;"utf-8")
