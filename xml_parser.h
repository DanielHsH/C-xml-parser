#ifndef XML_PARSER_H_DANIEL
#define XML_PARSER_H_DANIEL
/* 
* Coded By    : Daniel Herman Shmulyan
* Description : Daniels implementation of 2 C XML parsers, with zero memory usage (no memory allocations)
*               Definitions are based on schemas: http://www.w3.org/TR/xmlschema-1/, http://www.w3schools.com/xml/xml_attributes.asp
*				Important notations: Header, Comment, Tag, attribute, attributeValue, tagValue.  Example:
*				<?Header><!-- Comment -->
*				<Tag1 attribute1="attrValue1" attribute2="attrValue2">
*					<Tag2>tag2Value</Tag2>
*					<Tag3>tag2Value</Tag3>
*				</Tag1>
*
*				Implements 2 different XML parsers, 
*					1. 'strict' parser. Use it when the exact structure and schema is known in advanced. For example, software configuration files written in XML
*					2. Sequential parser. Use it when structure is unknown. Like HTML pages.
*/
/****************************************************************************/
// C / C++ Issues, if you decide to emmbed this code in C++.
#ifdef __cplusplus
	#define EXTERN_C_BEGIN extern "C" {
	#define EXTERN_C_END   }
#else
	#define EXTERN_C_BEGIN 
	#define EXTERN_C_END
#endif

typedef unsigned char UCHAR, MYBOOL;		// Just a boolean variable. Feel free to change to int.
#define _return_argument					// Meaningless, just for better code understanding of return arguments

EXTERN_C_BEGIN
/****************************************************************************/
/**************************** Strict XML parser *****************************/
/****************************************************************************/
// This parser is a non sequential XML parser. You can read specific tags of the XML without parsing the entire XML tree. 
// This parser does not support attributes of tags so it is good for simple structure XMLS (unsuitable for HTMLs)
// The down side ot this parser is that you must know the exact structure of the XML in advance. 
// How it works: You supply the path to a tag and the parser returns its value.
// For example: Input XML buffer and length:
// <ImageSizes>
//		<Rows>Y1</Rows><Cols>X1</Cols>
//      <Rows>Y2</Rows><Cols>X2</Cols>
// </ImageSizes>

const char*	XMLParserStrict_PeelHeader(									const char *pBuff);				// Skip XML header <?.....> if exists.
// If there are multiple occurances of a tag (like <rows> in the example) this method will give you the number of occurances so you can loop over them
// Example: XMLParserStrict_getNumTagsOccurances(XML_string,XML_Length,"Rows") returns 2. 
int			XMLParserStrict_getNumTagsOccurances(						const char *pBuff, const int iBuffLen, const char *tagName);				// Returns -1 if error

// Copies the value of a selected tag from 'pBuff' to allocated array 'dstStr'. The path of the tag in XML tree is described in ... arguments 
// ... - pairs of <TagName> and <OccuranceNumber>, <OccuranceNumber> is always > 0, (with NULL terminator)
// Example Call: XMLParserStrict_getTagValue(result, XML_string, XML_Length, TRUE, "ImageSizes", 1, "Rows", 2, NULL)
//			We requested the second occurance of "Rows" inside first occurance of "ImageSizes" - so the function will copy the string "Y2" to 'result'
//			Also returns the length of the copied string. If remove spaces is set to true, than peels of leading and trailing spaces in the result string.
int			XMLParserStrict_getTagValue( _return_argument char* dstStr, const char *pBuff, const int iBuffLen, MYBOOL removeSpaces, ...);

// Get a pointer to the value of a tag (instead of copying it), Usefull when you have deep trees and the amount of parameters to XMLParserStrict_getTagValue() is large
// Instead of calling	XMLParserStrict_getTagValue(result, XML_string   , XML_Length   , TRUE, "ImageSizes", 1, "Rows", 2, NULL);
// Replace by:			const char *ImageSizesPtr = XMLParserStrict_getTagValuePtr(&ImageSizesLen, XML_string, XML_Length, "ImageSizes", 1, NULL );
//						XMLParserStrict_getTagValue(result, ImageSizesPtr, ImageSizesLen, TRUE, "Rows", 2, NULL );
const char *XMLParserStrict_getTagValuePtr(_return_argument int* len,	const char *pBuff, const int iBuffLen, ...);

/****************************************************************************/
/*************************** Sequential XML parser **************************/
/****************************************************************************/
// This parser is designed to parse XMLs when you don't know they exact structure in advance, but know the rules upon which the XML was created.
// This parser can deal with Tags Attributs, but currently does not support father and son tags with identical name. Like: <txt><txt>AAA</txt></txt>
// This parser is sequantial - all the elements must be read from the beginning to the end. You can ignore specific tags but cannot jump over them.
// The code below DOES NOT define an XML parser but rather defines building blocks to create your own parser. For example, you need to supply XML rules upon which the parser will act
// To see an example of a parser, refer to the unitest file in the .cpp
// For example: Input XML buffer and length:
// <receipt>
//		<br/><!-- This is a comment after a new line tag -->
//		<img src="data:image/bmp;base64,Qk2eAAAAAAAAAD4AAA=">
//		<run scale="W100H200" align="center">Burger Perfect Florida Glen</run>
//		<run>    Total payment:     3.93</run>
//		<run><br/><!-- Comment --><br/></run>
//	</receipt>
struct XMLParserSeq;							// XML sequential parser

/****************************************************************************/
// Structure defining a a single XML tag
struct xmlTag{
	const char*name;							// The name of the tag, like "text" in <text>abc</text>
	const char*end;								// Closing of the tag. Typically should be </name>. for empty tags like <br/> - use NULL. 
	int nameLen;								// Length of the name of the tag. <text></text> - the name is 'text'. length 4. Imprtant for fast search of tags.
	void (*parse)(struct XMLParserSeq* P, void*v1, const struct xmlTag rules[]);	// Your function to parse the current xml tag. Receives 3 parameters
												// P - Pointer to parser object (not const since it keeps real time parsing info), v1-where to write output (pointer to result storge object), rules - XML rules (list of xmlTags)
												// If parsing function encounters an error, it should set the 'err' field of the XML parser to negative number. Otherwise set it to zero or positive.
	MYBOOL canHaveChildTags;					// Can this tag have tags inside it. Example: <!-- Comment --> or <br/> tags cannot have other tags inside. Exmaple of nested tags: <images><s>1</s><s>2</s></images>
};												// You should build an array of those structs for every tag you need, and probably an 'ignore' function for all other tags. The array of 'xmlTags' defines the rules of how to parse the XML.
typedef struct xmlTag xmlTag;

/****************************************************************************/
// Real time variables of the XML parser. Includes parsing indices. XML parser may have few such structures due to recursion (tag inside tag).
typedef struct{
	const struct xmlTag *curTag;				// During the parsing process. Points to the current tag structure
	const char *XMLend;							// Stores pointer to the end of the XML, to avoid buffer overrun. Parsing will never step beyond this point
	const char *curElem;						// Points to the current element being processed. Element may be a tag, a value, attribute, or any other XML part
	int			curLen;							// Length of the current element
} XMLParserSeq_tagPtr;

/****************************************************************************/
// Methods to for sequential parsing. Use them to inside your parse() functions to analyze the content of the tag, those methods operate on 'XMLParserSeq_tagPtr' but the arguments
// are defined as pointers/integers to give you more flexibility
const char*		XMLParserSeq_getNextTagName(	const char *pBuff, _return_argument int *len);							// Example: \n\n<ab>text</ab>		sets pointer to 'ab' and len=2
const xmlTag*	XMLParserSeq_findTagInfo(		const char* name,					int  len, const xmlTag *tagsList);	// Find the tag structure in your array according to its name
int				XMLParserSeq_getNextParamLen(	const char *pBuff);														// Example <ab p1="q">text</ab>		returns 6 - the length of p1="q"
const char*		XMLParserSeq_getNextParamName(	const char *pBuff, _return_argument int *len);							// Example <ab p1="q">text</ab>		sets pointer to 'p1' and returns 2
const char*		XMLParserSeq_getNextParamValue(	const char *pBuff, _return_argument int *len);							// Example <ab p1="q">text</ab>		sets pointer to "q" and returns 3
const char*		XMLParserSeq_getTagValueStart(  const char *pBuff, _return_argument int *len, const xmlTag* tag, const char* XMLend); // Example <ab>text</ab>		sets pointer to 'text' and returns 4
#define         XMLParserSeq_isCurElemEqualTo(R, tagName)	(!memcmp((R)->curElem,tagName,(R)->curLen))					// Test if current element is equal to a given string (tag name). Use when tag has many different parameters and you want to find which parameters this is
/****************************************************************************/
// Typical Definition of our XMLParserSeq parser. It does not use any memory allocations. You can extend it by C-inheritance
struct XMLParserSeq {									// Global XML representation
	void*		custom;									// Add a struct here to customize your own parser (C++ inheritance defined in C style)
	int			err;									// Should be >= 0. If parser fails to understand the XML, this filed will store the negative error code.
	XMLParserSeq_tagPtr r;								// Real time parsing indices. When Parser goes in to recursion (tag inside tag), only 'r' needs to be stored and restored again.
};														// No constructor/destructor supplied. Write your own methods.
typedef struct XMLParserSeq XMLParserSeq;
/****************************************************************************/
// Auxilliary methods to use inside your parse() functions. The macros below use 'XMLParserSeq' parser. Please read the unitest method to learn more, and see how parsers are implemented
#define XMLParserSeq_startWithCurrentTag(P)		P->r.curElem += P->r.curLen						// Define the parser & skip the name of the tag
#define XMLParserSeq_finishWithCurrentTag(P)	if   (P->r.curTag->end ==NULL) P->r.curElem++;	/* +1 since there is a trailing '>' like in <br/>. Put it at the end of your parse() functions */\
												else  P->r.curElem += (P->r.curLen + strlen(P->r.curTag->end)); /* Skip the last element + the termination tag: </tagname> */
#define XMLParserSeq_doneWithCurElement(P)		P->r.curElem += P->r.curLen						// Jump over the current element.
#define XMLParserSeq_startRecursion(P)			XMLParserSeq_tagPtr z_tmp45qd	= P->r;			// Store copy of 'r' on the stack. Before entering recursion
#define XMLParserSeq_finishRecursion(P)			P->r = z_tmp45qd;								// Restore copy of 'r' on the stack after exiting from recursion
#define XMLParserSeq_truncXMLtoCurrentTag(P)	P->r.XMLend = P->r.curElem + P->r.curLen;		// Set XMLend pointer to the end of current tag> Allowing us to recursively treat the value of a tag as an XML string (list of other tags).

// Parse sequential list of tags (XML file). For each tag we call its parse() function. parse() function is recursive for tags with canHaveChildTags==TRUE.
// Note: If parser enocunter an error, it immediately exits the parsing loop.
__inline void XMLParserSeq_run(XMLParserSeq* P, void*v1, const xmlTag rules[]){
	while ((P->r.curElem<P->r.XMLend)&&(P->err>=0)){											// While no errors and we havent reached the end of the xml
		P->r.curElem = XMLParserSeq_getNextTagName(P->r.curElem,&P->r.curLen);					// Extract the next 'tag' name
		P->r.curTag  = XMLParserSeq_findTagInfo(P->r.curElem, P->r.curLen, rules);				// Find the rule that defines how to parse the current tag
		P->r.curTag->parse(P,v1,rules);															// Apply the rule
	}
}

/****************************************************************************/
void XMLParsers_unitest(void);																	// Debugging function: used for testing the XML parsers mechanism.

EXTERN_C_END
#endif // H beggining
/****************************************************************************/
// EOF.