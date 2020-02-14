# 4d-plugin-xslt-v2

XSLT based on [libxml](http://xmlsoft.org/libxslt/).

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
||<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img width="32" height="32" src="https://user-images.githubusercontent.com/1725068/73986501-15964580-4981-11ea-9ac1-73c5cee50aae.png"> <img src="https://user-images.githubusercontent.com/1725068/73987971-db2ea780-4984-11ea-8ada-e25fb9c3cf4e.png" width="32" height="32" />

### Syntax

[miyako.github.io](https://miyako.github.io/2019/09/17/4d-plugin-xslt.html)

### Examples

```
$params:=New object
$params.str1:="'test'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder(Current resources folder)+"sample.xsl"
$xmlPath:=Get 4D folder(Current resources folder)+"apple.svg"

DOCUMENT TO BLOB($xmlPath;$xmlData)
DOCUMENT TO BLOB($xslPath;$xslData)

CONVERT FROM TEXT($xmlPath;"utf-8";$xmlFile)
CONVERT FROM TEXT($xslPath;"utf-8";$xslFile)

$xsltData:=XSLT Apply stylesheet ($xmlFile;$xslFile;$params)
$xslt:=Convert to text($xsltData;"utf-8")

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text($xsltData;"utf-8")
```
