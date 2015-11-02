// For documentation, see Header in H file
/****************************************************************************/
// Compiler Directive: Includes only ANSI C headers
#include "xml_parser.h" 
#include <stdio.h>					// printfs
#include <string.h>					// memory operations like memcmp() memset(), strlen(), strncpy()
#include <stdarg.h>					// using variable size arguments to functions.

// Disable with pragmas, old known C issues 
#pragma warning(disable : 4996)	// 'strcpy': This function or variable may be unsafe. Use strcpy_s
#pragma warning(disable : 4995) // 'sprintf': name was marked as #pragma deprecated

/****************************************************************************/
// Defines
#define SIZE_STR				(128)	// Maximal length of XML tag is up to 128 characters. </daniel> has length of 9
#define RETURN_ERR_VAL			-1		// The default numerical value returned to indicate an error

/****************************************************************************/
/****************************** Aux functions *******************************/
/****************************************************************************/
// Find sub string 'curTag' withing the first 'iSize' bytes of 'srcStr'. Note: that 'srcStr' can have zeros with the string, so regular search is not good
// Returns: pointer to the location of curTag in srcStr.
static const char *strstrn(const char *srcStr,const char *curTag,int iSize){
	const char *pStart = srcStr, *pEnd = srcStr, *pres;
	int	iCount = 0;

	for(;;){
		// Find the first null terminating char. This might not be the end of the string yet.
		while (*pEnd && iCount<iSize){
			pEnd++;
			iCount++;
		}
	
		if ((pres = strstr(pStart,curTag)) != NULL)
			return pres;								// Found, return the location

		if (iCount>= iSize)
			return pres;								// Pattern does not exist in the string
		pStart = pEnd+1;
		iCount++;										// Jump to the next part of the string
		pEnd++;
	}
}

/****************************************************************************/
// Copies at most 'maxLength' characters from 'src' to 'dst'. If removeSpaces is set, removes leading and terminating spaces.
// Return the length of 'dst' 
static int strcpyn_privte(char* dst, const char* src, int maxLength, MYBOOL removeSpaces){
	int res;
	if (removeSpaces){
		// Remove leading space
		while ((*src == ' ')||(*src == '\n')||(*src == '\t')){
			src++;
			maxLength--;
		}
	}
	// Copy
	strncpy(dst,src,maxLength);
	dst[maxLength] = '\0';
	res = (int)strlen(dst);
	if ((removeSpaces)&&(res!=0)){
		// Remove trailing spaces space
		char* end = dst+res-1;
		while ((*end == ' ')||(*end == '\n')||(*end == '\t')){
			*end-- = '\0';
		}
		res = (int)(end-dst)+1;
	}
 	return res;
}

/****************************************************************************/
// Convert tag name to valid XML open and close tags
#define generateXML_open_close_tags(curTag,curTagBegin,curTagEnd)	\
			sprintf(curTagBegin,"<%s>",curTag);	\
			sprintf(curTagEnd,"</%s>",curTag);

/****************************************************************************/
/**************************** Strict XML parser *****************************/
/****************************************************************************/
int XMLParserStrict_getTagValue(_return_argument char* pszDest, const char *pBuff, const int iBuffLen, MYBOOL removeSpaces, ...){
	const char *curLoc, *XMLend;					// current location in XML string and pointer to the end of the XML.
	char *curTag;									// current tag we are looking for. Like: daniel
	char curTagEnd[SIZE_STR],curTagBegin[SIZE_STR];	// String to store the xml open/close of current tags. Like <daniel> & </daniel>
	const char *curTagOpenLoc, *curTagCloseLoc;		// Location where we found the beginning of curent tag and location of the end.
	int i, neededTagOccurance;						// The i'th occurence of the tag that we need. Ususaly it is 1 (we search for first occurence) but can be any positive number.
	va_list ap;

	// Initialize 
	if (iBuffLen <= 0)
		return RETURN_ERR_VAL;
	va_start(ap, removeSpaces);
	curLoc = pBuff;
	XMLend = pBuff + iBuffLen;

	// Run over all arg-in assuming they are pairs of (char*,int) 
	curTag = va_arg(ap, char*);
	if (!curTag)
		return RETURN_ERR_VAL;
	neededTagOccurance = va_arg(ap, int);
	if (!neededTagOccurance)
		return RETURN_ERR_VAL;
	generateXML_open_close_tags(curTag,curTagBegin,curTagEnd);	

	// Find the i'th appearance of the current tag (curTag).
	for (i=0; (i < neededTagOccurance) && (*curLoc); i++){
		// Find the opening Tags and check for their validity
		curTagOpenLoc  = strstrn(curLoc, curTagBegin,(int)(XMLend-curLoc));
		curTagCloseLoc = strstrn(curLoc, curTagEnd  ,(int)(XMLend-curLoc)); 
		if ((curTagOpenLoc == NULL)||(curTagCloseLoc == NULL)||(curTagCloseLoc < curTagOpenLoc)||(curTagCloseLoc > XMLend))
			return RETURN_ERR_VAL;

		if (i < (neededTagOccurance-1)){
			// We have not found the needed appearance of the current tag. Skip to the next appearance. Like <dan>1</dan><dan>2</dan>
			curLoc = curTagCloseLoc + (int)strlen(curTagEnd);
			continue;
		}

		// We have reached the needed count of currenty tag. Proceed to next tag
		curTag = va_arg(ap, char*);
		if (!curTag){
			// Good. No more tags. We have completed our search
			curTagOpenLoc += (int)strlen(curTagBegin);
			return strcpyn_privte(pszDest,curTagOpenLoc, (int)(curTagCloseLoc-curTagOpenLoc),removeSpaces);
		}

		// We have to move to inner Tag
		curLoc = curTagOpenLoc + (int)strlen(curTagBegin);
		neededTagOccurance = va_arg(ap, int);
		if (!neededTagOccurance)
			return RETURN_ERR_VAL;
		generateXML_open_close_tags(curTag,curTagBegin,curTagEnd);
		i = -1; // Since at the end of the loop will become 0 due to i++. 0 appearnces of the new tag has been seen		
	} // For
	va_end(ap);
	return RETURN_ERR_VAL;
}

/****************************************************************************/
// Same As above but just return pointer instead of copying
const char* XMLParserStrict_getTagValuePtr(int* piAttStrLen, const char *pBuff, const int iBuffLen, ...){
	const char *curLoc, *XMLend;					// current location in XML string and pointer to the end of the XML.
	char *curTag;									// current tag we are looking for. Like: daniel
	char curTagEnd[SIZE_STR],curTagBegin[SIZE_STR];	// String to store the xml open/close of current tags. Like <daniel> & </daniel>
	const char *curTagOpenLoc, *curTagCloseLoc;		// Location where we found the beginning of curent tag and location of the end.
	int i, neededTagOccurance;						// The i'th occurence of the tag that we need. Ususaly it is 1 (we search for first occurence) but can be any positive number.
	va_list ap;

	// Initialize 
	if (iBuffLen <= 0)
		return NULL;
	va_start(ap, iBuffLen);
	curLoc = pBuff;
	XMLend = pBuff + iBuffLen;

	// Run over all arg-in assuming they are pairs of (char*,int) 
	curTag = va_arg(ap, char*);
	if (!curTag)
		return NULL;
	neededTagOccurance = va_arg(ap, int);
	if (!neededTagOccurance)
		return NULL;
	generateXML_open_close_tags(curTag,curTagBegin,curTagEnd);	

	// Find the i'th appearance of the current tag (curTag).
	for (i=0; (i < neededTagOccurance) && (*curLoc); i++){
		// Find the opening Tags and check for their validity
		curTagOpenLoc  = strstrn(curLoc, curTagBegin,(int)(XMLend-curLoc));
		curTagCloseLoc = strstrn(curLoc, curTagEnd  ,(int)(XMLend-curLoc)); 
		if ((curTagOpenLoc == NULL)||(curTagCloseLoc == NULL)||(curTagCloseLoc < curTagOpenLoc)||(curTagCloseLoc > XMLend))
			return NULL;

		if (i < (neededTagOccurance-1)){
			// We have not found the needed appearance of the current tag. Skip to the next appearance. Like <dan>1</dan><dan>2</dan>
			curLoc = curTagCloseLoc + (int)strlen(curTagEnd);
			continue;
		}

		// We have reached the needed count of currenty tag. Proceed to next tag
		curTag = va_arg(ap, char*);
		if (!curTag){
			curTagOpenLoc += (int)strlen(curTagBegin); 
			if (*curTagOpenLoc == '\n') 
				curTagOpenLoc++;
			*piAttStrLen = (int)(curTagCloseLoc-curTagOpenLoc);
			return curTagOpenLoc;
		}

		// We have to move to inner Tag
		curLoc = curTagOpenLoc + (int)strlen(curTagBegin);
		neededTagOccurance = va_arg(ap, int);
		if (!neededTagOccurance)
			return NULL;
		generateXML_open_close_tags(curTag,curTagBegin,curTagEnd);
		i = -1; // Since at the end of the loop will become 0 due to i++. 0 appearnces of the new tag has been seen		
	} // For
	va_end(ap);
	return NULL;
}

/****************************************************************************/
int XMLParserStrict_getNumTagsOccurances(const char *pBuff, const int iBuffLen, const char *tagName){
	// like XMLParserStrict_getTagValue() only instead of setting 'result' I increment a counter
	const char *curLoc, *XMLend;					// current location in XML string and pointer to the end of the XML.
	char curTagEnd[SIZE_STR],curTagBegin[SIZE_STR];	// String to store the xml open/close of current tags. Like <daniel> & </daniel>
	const char *curTagOpenLoc, *curTagCloseLoc;		// Location where we found the beginning of curent tag and location of the end.
	int i;

	// Initialize 
	if ((iBuffLen <= 0)||(tagName == NULL))
		return -1;
	curLoc = pBuff;
	XMLend = pBuff + iBuffLen;

	generateXML_open_close_tags(tagName,curTagBegin,curTagEnd);
	for (i=0; (*curLoc); i++){
		// Find the opening Tags and check for their validity
		curTagOpenLoc  = strstrn(curLoc, curTagBegin,(int)(XMLend-curLoc));
		curTagCloseLoc = strstrn(curLoc, curTagEnd  ,(int)(XMLend-curLoc)); 
		if ((curTagOpenLoc == NULL)||(curTagCloseLoc == NULL)||(curTagCloseLoc < curTagOpenLoc)||(curTagCloseLoc > (pBuff + iBuffLen)))
			return i;
		curLoc = curTagCloseLoc + (int)strlen(curTagEnd);
	}
	return i;
}

/****************************************************************************/
const char* XMLParserStrict_PeelHeader(const char *pBuff){
	const char *pTmp = pBuff;
	if ((pBuff[0] != '<')||(pTmp[1] != '?'))
		return pBuff;										// No header

	while (pBuff[0] != '>')
		pBuff++;
	return (pBuff+1);
}

/****************************************************************************/
/*************************** Sequential XML parser **************************/
/****************************************************************************/
const char *XMLParserSeq_getNextTagName(const char *pBuff, _return_argument int *len){
	const char* ret;
	while (*pBuff != '<') pBuff++;
	pBuff++;
	ret = pBuff;	
	while ((*pBuff != '>')&&(*pBuff != ' ')) pBuff++;
	*len = (int)(pBuff-ret);
	return ret;
}

/****************************************************************************/
const xmlTag *XMLParserSeq_findTagInfo(const char* nameStart, int nameLength, const xmlTag *tagsList){
	const xmlTag *curTag;
	for (curTag=tagsList; curTag->name != NULL; curTag++){
		if ((curTag->nameLen==nameLength)&&(!memcmp(nameStart,curTag->name,nameLength)))
			break;	// Found the tag.
	}
	return curTag;
}

/****************************************************************************/
int XMLParserSeq_getNextParamLen(const char *pBuff){
	const char *origPtr = pBuff;
	if (*pBuff=='>')									// No params.
		return 0;
	while (*pBuff == ' ') pBuff++;						// Skip leading spaces
	while ((*pBuff != ' ')&&(*pBuff != '>')) pBuff++;	// Skip text up to the next space or end of tag (for a single parameter)
	return (int)(pBuff-origPtr);
}

/****************************************************************************/
const char* XMLParserSeq_getNextParamName(const char *pBuff, int *paramLen){
	const char *origPtr;
	while (*pBuff == ' ') pBuff++;						// Skip leading spaces
	origPtr = pBuff;
	while (*pBuff != '=') pBuff++;
	*paramLen = (int)(pBuff-origPtr);
	return origPtr;
}

/****************************************************************************/
const char* XMLParserSeq_getNextParamValue(const char *pBuff, int *paramLen){
	const char *origPtr = (++pBuff);					// skip the '='
	while ((*pBuff != ' ')&&(*pBuff != '>')) pBuff++;
	*paramLen = (int)(pBuff-origPtr);
	return origPtr;
}

/****************************************************************************/
const char *XMLParserSeq_getTagValueStart(const char *pBuff, _return_argument int *len, const xmlTag* tag, const char* XMLend){
	const char *endPtr;
	while (*pBuff != '>') pBuff++;
	pBuff++;								// skip the '>'
	endPtr = strstrn(pBuff, tag->end, (int)(XMLend-pBuff));
	*len = (int)(endPtr-pBuff);
	return pBuff;
}

/****************************************************************************/
// EOF.
