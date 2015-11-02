// For documentation, see Header in H file
/****************************************************************************/
// Compiler Directive: Includes only ANSI C headers
#include "xml_parser.h" 
#include <stdio.h>					// printfs
#include <string.h>					// memory operations like memcmp() memset(), strlen(), strncpy()

/****************************************************************************/
// Defines
#ifndef FALSE
	#define	FALSE		(0)
#endif
#ifndef TRUE
	#define	TRUE		(1)
#endif

/****************************************************************************/
/*****************************  Unit testing ********************************/
/****************************************************************************/
// Method to parse the <txt> tag from the example xml
static void XMLParserSeq_init_private(XMLParserSeq* _this, const char* XMLbuffer){
	int iXMLLength;
	_this->err = 0;
	if (XMLbuffer==NULL){
		_this->err = -10;			// Not an error but cannot initialize
		return;
	}
	iXMLLength = (int)strlen(XMLbuffer);
	if (iXMLLength == 0){
		_this->err = -11;			// Not an error but cannot initialize
		return;
	}
	_this->r.XMLend = XMLbuffer + iXMLLength;
	_this->r.curElem = XMLParserStrict_getTagValuePtr(&_this->r.curLen, XMLbuffer, iXMLLength, "txt", 1, NULL);
	if (!_this->r.curElem){
		_this->err = -12;			// Cant find the container <txt>
		return;
	}
	XMLParserSeq_truncXMLtoCurrentTag(_this);	// Truncate XML to parse only between <XMLTAG_CONTAINER>....</XMLTAG_CONTAINER>.
	return;
}

/****************************************************************************/
static void XMLParserSeq_destroy_private(XMLParserSeq* _this){}		// No thing to do, no memory allocations were made

/****************************************************************************/
// Function to parse <BR> tag
static void XMLtag_BR_private(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	XMLParserSeq_startWithCurrentTag(P);
	fprintf((FILE*)v1,"-NewLine\n");
	XMLParserSeq_finishWithCurrentTag(P);
}

/****************************************************************************/
// Function to handle tags that should be ignored
static void XMLtag_ignore_private(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	XMLParserSeq_startWithCurrentTag(P);	
	P->r.curLen = XMLParserSeq_getNextParamLen(P->r.curElem);		
	while (P->r.curLen>0){
		XMLParserSeq_doneWithCurElement(P);							// Use a loop to ignore every parameters/arguments of the current tag 
		P->r.curLen = XMLParserSeq_getNextParamLen(P->r.curElem);
	}
	fprintf((FILE*)v1,"-Ignored: <%s>\n",P->r.curTag->name);
	XMLParserSeq_finishWithCurrentTag(P);							// Ignore the value of the current tag
	P->err = 0;	
}

/****************************************************************************/
// Function to handle exit tag
static void XMLtag_exit_private(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	XMLParserSeq_startWithCurrentTag(P);
	P->r.curElem	= P->r.XMLend;
	P->r.curLen		= 0;
	P->err			= 0;	
}

/****************************************************************************/
// Function to 'run' tag 
static void XMLtag_run_private(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	char allignment = 'l';											// Default value
	XMLParserSeq_startWithCurrentTag(P);
	P->r.curLen = XMLParserSeq_getNextParamLen(P->r.curElem);
	while (P->r.curLen>0){
		P->r.curElem = (char*)XMLParserSeq_getNextParamName(P->r.curElem, &P->r.curLen);
		if (XMLParserSeq_isCurElemEqualTo(&(P->r),"align"		)) allignment	= '?';	// Awaiting to be read
		XMLParserSeq_doneWithCurElement(P);
		P->r.curElem = (char*)XMLParserSeq_getNextParamValue(P->r.curElem,&P->r.curLen);
		if		(allignment == '?')	{ allignment = P->r.curElem[1];													}		// align="left | center | right"
		XMLParserSeq_doneWithCurElement(P);
		P->r.curLen = XMLParserSeq_getNextParamLen(P->r.curElem);
	}

	// Get the value of the tag
	P->r.curElem = XMLParserSeq_getTagValueStart(P->r.curElem,&P->r.curLen, P->r.curTag,P->r.XMLend);
	if ((P->r.curTag->canHaveChildTags)&&(P->r.curElem[0] == '<')){
		XMLParserSeq_startRecursion(P);			// We have children tags. Start recursion by defining the valu of the current tag as XML file and launching the parser. 
		XMLParserSeq_truncXMLtoCurrentTag(P);	// Truncate XML to parse only inside the current tag.
		XMLParserSeq_run(P,v1,rules);			// Lunch the parser
		XMLParserSeq_finishRecursion(P);		// Out from recursion
	} 
	else{
		int i;
		// Process the value of the tag. In our example, just print it
		fprintf((FILE*)v1,"-Align: %c.   -", allignment);
		for (i=0; i<P->r.curLen; i++)
			fprintf((FILE*)v1,"%c",P->r.curElem[i]);
		fprintf((FILE*)v1,"\n");
	}
	XMLParserSeq_finishWithCurrentTag(P);
}

/****************************************************************************/
// Function to handle illegal tags
static void XMLtag_illegal(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	XMLtag_exit_private(P,v1,rules);
	P->err = -1;
	fprintf((FILE*)v1,"Illegal tag!!!!");
}

/****************************************************************************/
void XMLParsers_unitest(void){
	FILE* fOut = stdout;
	// Test sequential parser
	{
		const char *inputXml=
"<txt>\
	<br/>\
	<img src=\"data:image/bmp;base64,Qk2eAAAAAAAAAD4AAA=\">\
	<!-- Comment -->\
	<run align=\"center\">Sunny Day</run>\
	<run><br/><!-- This is a tag within a tag --><br/></run>\
	<br/>\
</txt>";

		// List of all the XML tags with thier parsing functions.
		const xmlTag xmlRules[] = { 
			{"br/"	,	NULL		, 3	,&XMLtag_BR_private		, FALSE},	// <br/>. Cannot have inner tags
			{"run"	,	"</run>"	, 3	,XMLtag_run_private		, TRUE },	// <run parm1=val1 param2=val2>XXXXX</run>. Can have inner tags (childred)
			{"img"	,	NULL		, 3	,XMLtag_ignore_private	, FALSE},	// <img src="data:image/bmp;base64,{Base64 encoded image data}"/>; http://en.wikipedia.org/wiki/Base64
			{"!--"	,	NULL		, 3	,XMLtag_ignore_private	, FALSE},	// <!-- Comment -->
			{"/txt" ,	NULL		, 4	,XMLtag_exit_private	, FALSE},	// End of the XML
			{NULL	,	NULL		, 0	,XMLtag_exit_private	, FALSE}	// All other tags - unknown or illegal
		};
		XMLParserSeq parser;
		fprintf(fOut,"Parsing HTML: %s\n", inputXml);
		XMLParserSeq_init_private(&parser, inputXml);
		XMLParserSeq_run(&parser,fOut, xmlRules);							// Second Argument is 'stdout' because we will print to 'stdout' using fprintf and will not save the parsed data.
		XMLParserSeq_destroy_private(&parser);	
	}
	
	// Test strict parser
	{
		const char *inputXml =
		"<ImageSizes>\n"
		"   <Rows>Y1</Rows><Cols>X1</Cols>\n"
		"   <Rows>Y2</Rows><Cols>X2</Cols>\n"
		"</ImageSizes>\n";
		const int length = (int)strlen(inputXml); 
		int n, tagLen;
		char tagValue[100];
		const char *ImageSizesPtr;
		fprintf(fOut,"\n\nParsing XML: %s\n", inputXml);
		n = XMLParserStrict_getNumTagsOccurances(inputXml, length, "Rows");
		fprintf(fOut,"There are %d <Rows> tags in the XML\n", n); 
		tagLen = XMLParserStrict_getTagValue(tagValue, inputXml, length, TRUE, "ImageSizes", 1, "Rows", 2, NULL);
		fprintf(fOut,"There value of the second <Rows> tags is %s\n", tagValue); 
		ImageSizesPtr = XMLParserStrict_getTagValuePtr(&tagLen, inputXml, length, "ImageSizes", 1, NULL );
		fprintf(fOut,"Tags <ImageSizes> has %d characters, starting from character %d\n", tagLen, (int)(ImageSizesPtr-inputXml)); 
	}
}

/****************************************************************************/
// EOF.
