# C XML parser

plain C (not C++) implementation of 2 XML parsers, with zero memory usage (no memory allocations)
Definitions are based on schemas: http://www.w3.org/TR/xmlschema-1/, http://www.w3schools.com/xml/xml_attributes.asp

### Important notations: Header, Comment, Tag, attribute, attributeValue, tagValue.
<?Header><!-- Comment -->
<Tag1 attribute1="attrValue1" attribute2="attrValue2">
	<Tag2>tag2Value</Tag2>
	<Tag3>tag2Value</Tag3>
</Tag1>

###Implements 2 different XML parsers, 
1. Strict parser. Use it when the exact structure and schema is known in advanced. For example, software configuration files written in XML
2. Sequential parser. Use it when structure is unknown. Like HTML pages.