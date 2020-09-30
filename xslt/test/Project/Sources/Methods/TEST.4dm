//%attributes = {}
$params:=New object:C1471
$params.str1:="'stroke'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.xsl"
$xmlPath:=Get 4D folder:C485(Current resources folder:K5:16)+"apple.svg"

$text:="test"
TEXT TO DOCUMENT:C1237($xmlPath;$text)  //overwrite the apple.svg file

DOCUMENT TO BLOB:C525($xmlPath;$xmlData)
DOCUMENT TO BLOB:C525($xslPath;$xslData)

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text:C1012($xsltData;"utf-8")

  //in compiled mode the alert does not appear and 4D crashes.
ALERT:C41("XSLT Apply stylesheet done")