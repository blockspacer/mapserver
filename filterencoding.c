/**********************************************************************
 * $Id$
 *
 * Name:     filterencoding.c
 * Project:  MapServer
 * Language: C
 * Purpose:  OGC Filter Encoding implementation
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2003, Y. Assefa, DM Solutions Group Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.3  2003/09/02 22:59:06  assefa
 * Add classitem extrcat function for IsLike filter.
 *
 * Revision 1.2  2003/08/26 02:18:09  assefa
 * Add PropertyIsBetween and PropertyIsLike.
 *
 * Revision 1.1  2003/08/13 21:54:32  assefa
 * Initial revision.
 *
 *
 **********************************************************************/

#include "filterencoding.h"


/************************************************************************/
/*            FilterNode *PaserFilterEncoding(char *szXMLString)        */
/*                                                                      */
/*      Parses an Filter Encoding XML string and creates a              */
/*      FilterEncodingNodes corresponding to the string.                */
/*      Returns a pointer to the first node or NULL if                  */
/*      unsuccessfull.                                                  */
/*      Calling function should use FreeFilterEncodingNode function     */
/*      to free memeory.                                                */
/************************************************************************/
FilterEncodingNode *PaserFilterEncoding(char *szXMLString)
{
    CPLXMLNode *psRoot, *psChild, *psFilter = NULL;
    FilterEncodingNode *psFilterNode = NULL;

    if (szXMLString == NULL || strlen(szXMLString) <= 0 ||
        (strstr(szXMLString, "<Filter>") == NULL))
      return NULL;

    psRoot = CPLParseXMLString(szXMLString);
    if( psRoot == NULL)
       return NULL;

/* -------------------------------------------------------------------- */
/*      get the root element (Filter).                                  */
/* -------------------------------------------------------------------- */
    psChild = psRoot;
    psFilter = NULL;
    while( psChild != NULL )
    {
        if (psChild->eType == CXT_Element &&
            EQUAL(psChild->pszValue,"Filter"))
        {
            psFilter = psChild;
            break;
        }
        else
          psChild = psChild->psNext;
    }

    if (!psFilter)
      return NULL;

    psChild = psFilter->psChild;
    if (psChild && IsSupportedFilterType(psChild))
    {
        psFilterNode = CreateFilerEncodingNode();
        
        InsertElementInNode(psFilterNode, psChild);
    }

    return psFilterNode;
}

/************************************************************************/
/*                          FreeFilterEncodingNode                      */
/*                                                                      */
/*      recursive freeing of Filer Encoding nodes.                      */
/************************************************************************/
void FreeFilterEncodingNode(FilterEncodingNode *psFilterNode)
{
    if (psFilterNode)
    {
        if (psFilterNode->psLeftNode)
        {
            FreeFilterEncodingNode(psFilterNode->psLeftNode);
            psFilterNode->psLeftNode = NULL;
        }
        if (psFilterNode->psRightNode)
        {
            FreeFilterEncodingNode(psFilterNode->psRightNode);
            psFilterNode->psRightNode = NULL;
        }

        free(psFilterNode);
    }
}


/************************************************************************/
/*                         CreateFilerEncodingNode                      */
/*                                                                      */
/*      return a FilerEncoding node.                                    */
/************************************************************************/
FilterEncodingNode *CreateFilerEncodingNode()
{
    FilterEncodingNode *psFilterNode = NULL;

    psFilterNode = 
      (FilterEncodingNode *)malloc(sizeof (FilterEncodingNode));
    psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    psFilterNode->pszValue = NULL;
    psFilterNode->pOther = NULL;
    psFilterNode->psLeftNode = NULL;
    psFilterNode->psRightNode = NULL;

    return psFilterNode;
}


/************************************************************************/
/*                           InsertElementInNode                        */
/*                                                                      */
/*      Utility function to parse an XML node and transfter the         */
/*      contennts into the Filer Encoding node structure.               */
/************************************************************************/
void InsertElementInNode(FilterEncodingNode *psFilterNode,
                         CPLXMLNode *psXMLNode)
{
    int nStrLength = 0;
    char *pszTmp = NULL;

    if (psFilterNode && psXMLNode)
    {
        psFilterNode->pszValue = strdup(psXMLNode->pszValue);
        psFilterNode->psLeftNode = NULL;
        psFilterNode->psRightNode = NULL;
        
/* -------------------------------------------------------------------- */
/*      Logical filter. AND, OR and NOT are supported. Example of       */
/*      filer using logical filters :                                   */
/*      <Filter>                                                        */
/*        <And>                                                         */
/*          <PropertyIsGreaterThan>                                     */
/*            <PropertyName>Person/Age</PropertyName>                   */
/*            <Literal>50</Literal>                                     */
/*          </PropertyIsGreaterThan>                                    */
/*          <PropertyIsEqualTo>                                         */
/*             <PropertyName>Person/Address/City</PropertyName>         */
/*             <Literal>Toronto</Literal>                               */
/*          </PropertyIsEqualTo>                                        */
/*        </And>                                                        */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
        if (IsLogicalFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_LOGICAL;
            if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
                strcasecmp(psFilterNode->pszValue, "OR") == 0)
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild);
                    psFilterNode->psRightNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psRightNode, 
                                        psXMLNode->psChild->psNext);
                }
            }
            else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
            {
                if (psXMLNode->psChild)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild); 
                }
            }                           
        }//end if is logical
/* -------------------------------------------------------------------- */
/*      Spatial Filter. Only BBox is supported. Eg of Filer using       */
/*      BBOX :                                                          */
/*      <Filter>                                                        */
/*       <BBOX>                                                         */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326�>*/
/*          <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*        </gml:Box>                                                    */
/*       </BBOX>                                                        */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
        else if (IsSpatialFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_SPATIAL;
            if (psXMLNode->psChild && psXMLNode->psChild->psNext &&
                strcasecmp(psXMLNode->psChild->pszValue, "PropertyName") == 0 &&
                strcasecmp(psXMLNode->psChild->psNext->pszValue,"gml:Box") == 0)
                
            {
                psFilterNode->psLeftNode = CreateFilerEncodingNode();

                psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                psFilterNode->psLeftNode->pszValue = 
                  psXMLNode->psChild->pszValue;

                //TODO extract SRS value
                if (psXMLNode->psChild->psNext->psChild)
                {
                    psFilterNode->psRightNode = CreateFilerEncodingNode();

                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
                
                    psFilterNode->psRightNode->pszValue = 
                      psXMLNode->psChild->psNext->psChild->pszValue;
                }
            }
        }//end of is spatial
/* -------------------------------------------------------------------- */
/*      Comparison Filter                                               */
/* -------------------------------------------------------------------- */
        else if (IsComparisonFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_COMPARISON;
/* -------------------------------------------------------------------- */
/*      binary comaparison types. Example :                             */
/*                                                                      */
/*      <Filter>                                                        */
/*        <PropertyIsEqualTo>                                           */
/*          <PropertyName>SomeProperty</PropertyName>                   */
/*          <Literal>100</Literal>                                      */
/*        </PropertyIsEqualTo>                                          */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            if (IsBinaryComparisonFilterType(psXMLNode->pszValue))
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext &&
                    strcasecmp(psXMLNode->psChild->pszValue, "PropertyName") == 0 &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue,"Literal") == 0)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();

                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    if ( psXMLNode->psChild->psChild)
                      psFilterNode->psLeftNode->pszValue = 
                        strdup(psXMLNode->psChild->psChild->pszValue);

                    psFilterNode->psRightNode = CreateFilerEncodingNode();

                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    if (psXMLNode->psChild->psNext->psChild)
                      psFilterNode->psRightNode->pszValue = 
                        strdup(psXMLNode->psChild->psNext->psChild->pszValue);
                }
            }
/* -------------------------------------------------------------------- */
/*      PropertyIsBetween filter : extract property name and boudary    */
/*      values. The boundary  values are stored in the right            */
/*      node. The values are separated by a semi-column (;)             */
/*      Eg of Filter :                                                  */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary>400</LowerBoundary>                           */
/*         <UpperBoundary>800</UpperBoundary>                           */
/*      </PropertyIsBetween>                                            */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsBetween") == 0)
            {
                if (psXMLNode->psChild &&
                    strcasecmp(psXMLNode->psChild->pszValue, 
                               "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext &&  
                      strcasecmp(psXMLNode->psChild->psNext->pszValue, 
                               "LowerBoundary") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, 
                                "UpperBoundary") == 0)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    if (psXMLNode->psChild->psChild)
                      psFilterNode->psLeftNode->pszValue = 
                        strdup(psXMLNode->psChild->psChild->pszValue);

                    psFilterNode->psRightNode = CreateFilerEncodingNode();
                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BOUNDARY;
                    if (psXMLNode->psChild->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psChild)
                    {
                        nStrLength = 
                          strlen(psXMLNode->psChild->psNext->psChild->pszValue);
                        nStrLength +=
                          strlen(psXMLNode->psChild->psNext->psNext->
                                 psChild->pszValue);

                        nStrLength += 2; //adding a ; between bounary values
                         psFilterNode->psRightNode->pszValue = 
                           (char *)malloc(sizeof(char)*(nStrLength));
                         strcpy( psFilterNode->psRightNode->pszValue,
                                 psXMLNode->psChild->psNext->psChild->pszValue);
                         strcat(psFilterNode->psRightNode->pszValue, ";");
                         strcat(psFilterNode->psRightNode->pszValue,
                                psXMLNode->psChild->psNext->psNext->psChild->pszValue); 
                         psFilterNode->psRightNode->pszValue[nStrLength-1]='\0';
                    }
                         
                  
                }
            }//end of PropertyIsBetween 
/* -------------------------------------------------------------------- */
/*      PropertyIsLike                                                  */
/*                                                                      */
/*      <Filter>                                                        */
/*      <PropertyIsLike wildCard="*" singleChar="#" escapeChar="!">     */
/*      <PropertyName>LAST_NAME</PropertyName>                          */
/*      <Literal>JOHN*</Literal>                                        */
/*      </PropertyIsLike>                                               */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsLike") == 0)
            {
                if (psXMLNode->psChild &&  
                    strcasecmp(psXMLNode->psChild->pszValue, "wildCard") == 0 &&
                    psXMLNode->psChild->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue, "singleChar") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, "escapeChar") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->pszValue, "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->psNext->pszValue, "Literal") == 0)
                {
/* -------------------------------------------------------------------- */
/*      Get the wildCard, the singleChar and the escapeChar used.       */
/* -------------------------------------------------------------------- */
                    psFilterNode->pOther = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "wildCard", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "singleChar", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escapeChar", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar = 
                        strdup(pszTmp);
/* -------------------------------------------------------------------- */
/*      Create left and right node for the attribute and the value.     */
/* -------------------------------------------------------------------- */
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();

                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    
                    if (psXMLNode->psChild->psNext->psNext->psNext->psChild)
                      psFilterNode->psLeftNode->pszValue = 
                        strdup(psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue);

                    psFilterNode->psRightNode = CreateFilerEncodingNode();

                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    if (psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild)
                    psFilterNode->psRightNode->pszValue = 
                        strdup(psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue);
                }
            }
        }
            
    }
}

 
/************************************************************************/
/*            int IsLogicalFilterType((char *pszValue)                  */
/*                                                                      */
/*      return TRUE if the value of the node is of logical filter       */
/*       encoding type.                                                 */
/************************************************************************/
int IsLogicalFilterType(char *pszValue)
{
    if (pszValue)
    {
        if (strcasecmp(pszValue, "AND") == 0 ||
            strcasecmp(pszValue, "OR") == 0 ||
             strcasecmp(pszValue, "NOT") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*         int IsBinaryComparisonFilterType(char *pszValue)             */
/*                                                                      */
/*      Binary comparison filter type.                                  */
/************************************************************************/
int IsBinaryComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (strcasecmp(pszValue, "PropertyIsEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsNotEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThanOrEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThanOrEqualTo") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*            int IsComparisonFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of comparison filter    */
/*      encoding type.                                                  */
/************************************************************************/
int IsComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (IsBinaryComparisonFilterType(pszValue) ||  
             strcasecmp(pszValue, "PropertyIsLike") == 0 ||  
             strcasecmp(pszValue, "PropertyIsBetween") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}


/************************************************************************/
/*            int IsSpatialFilterType(char *pszValue)                   */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int IsSpatialFilterType(char *pszValue)
{
    if (pszValue)
    {
        if ( strcasecmp(pszValue, "BBOX") == 0)
          return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*           int IsSupportedFilterType(CPLXMLNode *psXMLNode)           */
/*                                                                      */
/*      Verfify if the value of the node is one of the supported        */
/*      filter type.                                                    */
/************************************************************************/
int IsSupportedFilterType(CPLXMLNode *psXMLNode)
{
    if (psXMLNode)
    {
        if (IsLogicalFilterType(psXMLNode->pszValue) ||
            IsSpatialFilterType(psXMLNode->pszValue) ||
            IsComparisonFilterType(psXMLNode->pszValue))
          return MS_TRUE;
    }

    return MS_FALSE;
}
            
            

/************************************************************************/
/*                     GetMapserverExpressionClassItem                  */
/*                                                                      */
/*      Check if one of the node is a PropertyIsLike filter node. If    */
/*      that is the case return the value that will be used as a        */
/*      ClassItem in mapserver.                                         */
/*      NOTE : only the first one is used.                              */
/************************************************************************/
char *GetMapserverExpressionClassItem(FilterEncodingNode *psFilterNode)
{
    char *pszReturn = NULL;

    if (!psFilterNode)
      return NULL;

    if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    {
        if (psFilterNode->psLeftNode)
          return psFilterNode->psLeftNode->pszValue;
    }
    else
    {
        pszReturn = GetMapserverExpressionClassItem(psFilterNode->psLeftNode);
        if (pszReturn)
          return pszReturn;
        else
          return  GetMapserverExpressionClassItem(psFilterNode->psRightNode);
    }
}
/************************************************************************/
/*                          GetMapserverExpression                      */
/*                                                                      */
/*      Return a mapserver expression base on the Filer encoding nodes. */
/************************************************************************/
char *GetMapserverExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;

    if (!psFilterNode)
      return NULL;

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (IsBinaryComparisonFilterType(psFilterNode->pszValue))
            {
                pszExpression = GetBinaryComparisonExpresssion(psFilterNode);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsBetween") == 0)
            {
                 pszExpression = GetIsBetweenComparisonExpresssion(psFilterNode);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsLike") == 0)
            {
                 pszExpression = GetIsLikeComparisonExpresssion(psFilterNode);
            }
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
            strcasecmp(psFilterNode->pszValue, "OR") == 0)
        {
            pszExpression = GetLogicalComparisonExpresssion(psFilterNode);
        }
        else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
        {       
            pszExpression = GetLogicalComparisonExpresssion(psFilterNode);
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL)
    {
        //TODO
    }
            
    return pszExpression;
}
  
/************************************************************************/
/*                            GetNodeExpression                         */
/*                                                                      */
/*      Return the expresion for a specific node.                       */
/************************************************************************/
char *GetNodeExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;
    if (!psFilterNode)
      return NULL;

    if (IsLogicalFilterType(psFilterNode->pszValue))
      pszExpression = GetLogicalComparisonExpresssion(psFilterNode);
    else if (IsComparisonFilterType(psFilterNode->pszValue))
    {
        if (IsBinaryComparisonFilterType(psFilterNode->pszValue))
          pszExpression = GetBinaryComparisonExpresssion(psFilterNode);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
          pszExpression = GetIsBetweenComparisonExpresssion(psFilterNode);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
          pszExpression = GetIsLikeComparisonExpresssion(psFilterNode);
    }

    return pszExpression;
}


/************************************************************************/
/*                     GetLogicalComparisonExpresssion                  */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *GetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char *pszTmp = NULL;
    szBuffer[0] = '\0';

    if (!psFilterNode || !IsLogicalFilterType(psFilterNode->pszValue))
      return NULL;

    
/* -------------------------------------------------------------------- */
/*      OR and AND                                                      */
/* -------------------------------------------------------------------- */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
        strcat(szBuffer, " (");
        pszTmp = GetNodeExpression(psFilterNode->psLeftNode);
        if (!pszTmp)
          return NULL;

        strcat(szBuffer, pszTmp);
        strcat(szBuffer, " ");
        strcat(szBuffer, psFilterNode->pszValue);
        strcat(szBuffer, " ");
        pszTmp = GetNodeExpression(psFilterNode->psRightNode);
        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);
        strcat(szBuffer, ") ");
    }
/* -------------------------------------------------------------------- */
/*      NOT                                                             */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && 
             strcasecmp(psFilterNode->pszValue, "NOT") == 0)
    {
        strcat(szBuffer, " (NOT ");
        pszTmp = GetNodeExpression(psFilterNode->psLeftNode);
        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);
        strcat(szBuffer, ") ");
    }
    else
      return NULL;
    
    return strdup(szBuffer);
    
}

    
    
/************************************************************************/
/*                      GetBinaryComparisonExpresssion                  */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *GetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    int i, bString, nLenght = 0;

    szBuffer[0] = '\0';
    if (!psFilterNode || !IsBinaryComparisonFilterType(psFilterNode->pszValue))
      return NULL;

/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (psFilterNode->psRightNode->pszValue)
    {
        nLenght = strlen(psFilterNode->psRightNode->pszValue);
        for (i=0; i<nLenght; i++)
        {
            if (psFilterNode->psRightNode->pszValue[i] < '0' ||
                psFilterNode->psRightNode->pszValue[i] > '9')
            {
                bString = 1;
                break;
            }
        }
    }

    if (bString)
      strcat(szBuffer, " ('[");
    else
      strcat(szBuffer, " ([");
    //attribute

    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    if (bString)
      strcat(szBuffer, "]' ");
    else
      strcat(szBuffer, "] "); 
    

    //logical operator
    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
      strcat(szBuffer, "=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      strcat(szBuffer, "!="); 
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      strcat(szBuffer, "<");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      strcat(szBuffer, ">");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqualTo") == 0)
      strcat(szBuffer, "<=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqualTo") == 0)
      strcat(szBuffer, ">=");
    
    strcat(szBuffer, " ");
    
    //value
    if (bString)
      strcat(szBuffer, "'");
    strcat(szBuffer, psFilterNode->psRightNode->pszValue);
    if (bString)
      strcat(szBuffer, "'");
    strcat(szBuffer, ") ");

    return strdup(szBuffer);
}


/************************************************************************/
/*                    GetIsBetweenComparisonExpresssion                 */
/*                                                                      */
/*      Build expresssion for IsBteween Filter.                         */
/************************************************************************/
char *GetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char **aszBounds = NULL;
    int nBounds = 0;
    int i, bString, nLenght = 0;

    szBuffer[0] = '\0';
    if (!psFilterNode ||
        !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
      return NULL;

    if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
      return NULL;

/* -------------------------------------------------------------------- */
/*      Get the bounds value which are stored like boundmin;boundmax    */
/* -------------------------------------------------------------------- */
    aszBounds = split(psFilterNode->psRightNode->pszValue, ';', &nBounds);
    if (nBounds != 2)
      return NULL;
/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (aszBounds[0])
    {
        nLenght = strlen(aszBounds[0]);
        for (i=0; i<nLenght; i++)
        {
            if (aszBounds[0][i] < '0' ||
                aszBounds[0][i] > '9')
            {
                bString = 1;
                break;
            }
        }
    }
    if (!bString)
    {
        if (aszBounds[1])
        {
            nLenght = strlen(aszBounds[1]);
            for (i=0; i<nLenght; i++)
            {
                if (aszBounds[1][i] < '0' ||
                    aszBounds[1][i] > '9')
                {
                    bString = 1;
                    break;
                }
            }
        }
    }
        

/* -------------------------------------------------------------------- */
/*      build expresssion.                                              */
/* -------------------------------------------------------------------- */
    if (bString)
      strcat(szBuffer, " ('[");
    else
      strcat(szBuffer, " ([");

    //attribute
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);

    if (bString)
      strcat(szBuffer, "]' ");
    else
      strcat(szBuffer, "] ");
        
    
    strcat(szBuffer, " >= ");
    strcat(szBuffer, aszBounds[0]);
    strcat(szBuffer, " AND ");

    if (bString)
      strcat(szBuffer, " '[");
    else
      strcat(szBuffer, " ["); 

    //attribute
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    
    if (bString)
      strcat(szBuffer, "]' ");
    else
      strcat(szBuffer, "] ");
    
    strcat(szBuffer, " <= ");
    strcat(szBuffer, aszBounds[1]);
    strcat(szBuffer, ")");
     
    
    return strdup(szBuffer);
}
    
/************************************************************************/
/*                      GetIsLikeComparisonExpresssion                  */
/*                                                                      */
/*      Build expression for IsLike filter.                             */
/************************************************************************/
char *GetIsLikeComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char *pszValue = NULL;
    
    char *pszWild = NULL;
    char *pszSingle = NULL;
    char *pszEscape = NULL;
    
    int nLength, i, iBuffer = 0;
    if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
        !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
      return NULL;

    pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
    pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
    pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;

    if (!pszWild || strlen(pszWild) == 0 ||
        !pszSingle || strlen(pszSingle) == 0 || 
        !pszEscape || strlen(pszEscape) == 0)
      return NULL;

 
/* -------------------------------------------------------------------- */
/*      Use only the right node (which is the value), to build the      */
/*      regular expresssion. The left node will be used to build the    */
/*      classitem.                                                      */
/* -------------------------------------------------------------------- */
    szBuffer[0] = '/';
    pszValue = psFilterNode->psRightNode->pszValue;
    nLength = strlen(pszValue);
    iBuffer = 1;
    for (i=0; i<nLength; i++)
    {
        if (pszValue[i] != pszWild[0] && 
            pszValue[i] != pszSingle[0] &&
            pszValue[i] != pszEscape[0])
        {
            szBuffer[iBuffer] = pszValue[i];
            iBuffer++;
            szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszSingle[0])
        {
             szBuffer[iBuffer] = '.';
             iBuffer++;
             szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszEscape[0])
        {
            if (i<nLength-1)
            {
                szBuffer[iBuffer] = pszValue[i+1];
                iBuffer++;
                szBuffer[iBuffer] = '\0';
            }
        }
        else if (pszValue[i] == pszWild[0])
        {
            strcat(szBuffer, "[a-z,A-Z]*");
            iBuffer+=10;
            szBuffer[iBuffer] = '\0';
        }
    }   
    szBuffer[iBuffer] = '/';
    szBuffer[iBuffer+1] = '\0';
    
        
    return strdup(szBuffer);
}
