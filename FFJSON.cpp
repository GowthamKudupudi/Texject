/* 
 * File:	  FFJSON.cpp
 * Author: Satya Gowtham Kudupudi
 * 
 * Created on November 29, 2013, 4:29 PM
 */

/*
 * To do:
 * 1. remove escape characters from string
 */
#include <string>
#ifndef __APPLE__
#ifndef __MACH__
#endif
#endif
#include <math.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <map>
#include <exception>
#include <algorithm>
#include <ferrybase/FerryTimeStamp.h>
#include <ferrybase/myconverters.h>
#include <ferrybase/mystdlib.h>
#include <logger.h>
#include <stdexcept>

#include "FFJSON.h"

using namespace std;

const char FFJSON::OBJ_STR[15][15] = {
	"UNDEFINED",
	"BOOL",
	"BINARY",
	"NUMBER",
	"TIME",
	"STRING",
	"XML",
	"SET_TYPE",
	"NEW_SET_MEMBER",
	"ARRAY",
	"OBJECT",
	"BIG_OBJECT", //number of members are greater than MAX_ORDERED_MEMBERS; Its
	"LINK",		  //used only by Iterator.
	"DLINK",
	"NUL"
};
map<FFJSON*, set<FFJSON::FFJSONIterator> > FFJSON::sm_mUpdateObjs;

FFJSON::FFJSON () {
	//	  type = UNDEFINED;
	//	  qtype = NONE;
	//	  etype = ENONE;
	flags = 0;
	size = 0;
	val.number = 0;
	val.boolean = false;
}

FFJSON::FFJSON (OBJ_TYPE t) {
	//s//
	//	  type = UNDEFINED;
	//	  qtype = NONE;
	//	  etype = ENONE;
	flags = 0;
	if (t == OBJECT) {
		setType(OBJECT);
		val.pairs = new map<string, FFJSON*>;
		FeaturedMember fmMapSequence;
		fmMapSequence.m_pvpsMapSequence = new vector<map<string, FFJSON*>::
																	iterator>();
		insertFeaturedMember(fmMapSequence, FM_MAP_SEQUENCE);
	} else if (t == ARRAY) {
		setType(ARRAY);
		val.array = new vector<FFJSON*>();
	} else if (t == STRING) {
		setType(STRING);
	} else if (t == XML) {
		setType(XML);
	} else if (t == NUMBER) {
		setType(NUMBER);
	} else if (t == TIME) {
		setType(TIME);
		val.m_pFerryTimeStamp = new FerryTimeStamp();
	} else if (t == BOOL) {
		setType(BOOL);
	} else if (t == NUL) {
		setType(NUL);
	} else {
		setType(UNDEFINED);
	}
}

FFJSON::FFJSON (const FFJSON& orig, COPY_FLAGS cf, FFJSONPObj* pObj) {
	copy(orig, cf, pObj);
}

FFJSON::FFJSON (const string& ffjson, int* ci, int indent,
					FFJSON::FFJSONPObj* pObj) : size(0), flags(0) {
	//FFJSON::FFJSONPObj* pObj) {
	init(ffjson, ci, indent, pObj);
}

void FFJSON::copy (const FFJSON& orig, COPY_FLAGS cf, FFJSONPObj* pObj) {
	if (!(isType(OBJECT)||isType(ARRAY))) {
		freeObj();
		flags = 0;
		setType(orig.getType());
		if (isType(OBJECT)) {
			//TODO: MAX
			FeaturedMember fmMapSequence;
			fmMapSequence.m_pvpsMapSequence =
				new vector<map<string,FFJSON*>::iterator>();
			insertFeaturedMember(fmMapSequence, FM_MAP_SEQUENCE);
			val.pairs = new map<string, FFJSON*>();
		}
		size = orig.size;
	}
	switch (getType()) {
		case NUMBER:
			val.number = orig.val.number;
			if (orig.isEFlagSet(PRECISION)) {
				setEFlag(PRECISION);
				FeaturedMember cFM = orig.getFeaturedMember(FM_PRECISION);
				insertFeaturedMember(cFM, FM_PRECISION);
			}
			break;
		case STRING: {
			val.string = new char[orig.size + 1];
			memcpy(val.string, orig.val.string, orig.size);
			val.string[orig.size] = '\0';
			size = orig.size;
			if (cf & COPY_EFLAGS) {
				setEFlag(orig.getEFlags());
			}
			FeaturedMember fmWidth;
			fmWidth.width = orig.getFeaturedMember(FM_WIDTH).width;
			insertFeaturedMember(fmWidth, FM_WIDTH);
			break;
		}
		case XML:
			val.string = new char[orig.size + 1];
			memcpy(val.string, orig.val.string, orig.size);
			val.string[orig.size] = '\0';
			size = orig.size;
			if (cf & COPY_EFLAGS) {
				setEFlag(orig.getEFlags());
			}
			break;
		case BOOL:
			val.boolean = orig.val.boolean;
			break;
		case OBJECT: {
			FeaturedMember fmMapSequence = getFeaturedMember(FM_MAP_SEQUENCE);
			map<string, FFJSON*>::iterator i;
			FeaturedMember fmOrigMapSequence = orig.getFeaturedMember(FM_MAP_SEQUENCE);
			int iMapSeqIndexer = 0;
			if (fmOrigMapSequence.m_pvpsMapSequence) {
				if (iMapSeqIndexer < fmOrigMapSequence.m_pvpsMapSequence->size()) {
					i = (*fmOrigMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
				} else {
					i = orig.val.pairs->end();
				}
			} else {
				i = orig.val.pairs->begin();
			}
			FFJSONPObj pLObj;
			pLObj.pObj = pObj;
			pLObj.value = this;
			while (i != orig.val.pairs->end()) {
				FFJSON* fo = NULL;
				pLObj.name = &i->first;
				map<string, FFJSON*>::iterator ii = val.pairs->find(i->first);
				if (ii!=val.pairs->end() &&
					 ((cf && COPY_SHALLOW) != COPY_SHALLOW)) {
					ii->second->copy(*i->second, cf, &pLObj);
				} else {
					if (((cf && COPY_SHALLOW) == COPY_SHALLOW)) {
						delete ii->second;
						fo= new FFJSON();
						*fo= &(*i->second);
					} else {
						fo = new FFJSON(*i->second, cf, &pLObj);
					}//pair<map<string, FFJSON*>::iterator, bool> prNew = val.pairs->insert(pair<string, FFJSON*>(i->first, new FFJSON(*i->second, cf, &pLObj)));
					
				}
				if (fo && ((cf == COPY_QUERIES && !fo->isQType(QUERY_TYPE::NONE))
							  || !fo->isType(UNDEFINED))) {
					pair < map<string, FFJSON*>::iterator, bool> prNew = val.
						pairs->insert(pair<string, FFJSON*>(i->first, fo));
					if (size < MAX_ORDERED_MEMBERS) {
						fmMapSequence.m_pvpsMapSequence->push_back(prNew.first);
					} else if (fmMapSequence.m_pvpsMapSequence) {
						deleteFeaturedMember(FM_MAP_SEQUENCE);
					}
				} else {
					delete fo;
				}
				if (fmOrigMapSequence.m_pvpsMapSequence) {
					if (iMapSeqIndexer <
						 fmOrigMapSequence.m_pvpsMapSequence->size()) {
						i= (*fmOrigMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
					} else {
						i= orig.val.pairs->end();
					}
				} else {
					++i;
				}
			}
			if (val.pairs->size() == 0) {
				delete val.pairs;
				val.pairs = NULL;
				setType(UNDEFINED);
			};
			break;
		}
		case ARRAY: {
			int i = 0;
			bool matter = false;
			FFJSONPObj pLObj;
			pLObj.pObj = pObj;
			pLObj.value = this;
			if (val.array==nullptr) {
				this->init("[]");
			}
			while (i < orig.val.array->size()) {
				FFJSON* fo = NULL;
				string index = to_string(i);
				pLObj.name = &index;
				if ((*orig.val.array)[i] != NULL)
					(*this)[i] = (*(*orig.val.array)[i]);
				++i;
				matter=true;
			}
			if (!matter) {
				freeObj();
			}
			break;
		}
		case SET_TYPE: {
			size=0;
			val.setPtr = new ffset();
			for (FFJSON* fp : *orig.val.setPtr) {
				FFJSON* newcopy = new FFJSON(*fp);
				if (val.setPtr->insert(newcopy).second)
					++size;
				else
					delete newcopy;
			}
			break;
		}
		case LINK: {
			vector<string>* ln =
				new vector<string>(*orig.getFeaturedMember(FM_LINK).link);
			FeaturedMember fm;
			fm.link = ln;
			insertFeaturedMember(fm, FM_LINK);
			val.fptr = orig.val.fptr;
			break;
		}
		case TIME:
			val.m_pFerryTimeStamp =
				new FerryTimeStamp(*orig.val.m_pFerryTimeStamp);
			break;
		case NUL:
			setType(NUL);
			size = 0;
			val.boolean = false;
			break;
		default:
			if ((cf == COPY_QUERIES&&!isQType(QUERY_TYPE::NONE)) &&
				 isType(UNDEFINED)) {
				setQType(orig.getQType());
			} else {
				setType(UNDEFINED);
				val.boolean = false;
			}
			break;
	}
	if (orig.isEFlagSet(EXTENDED) && !isType(STRING)) {
		FFJSON* pOrigParent = orig.getFeaturedMember(FM_PARENT).m_pParent;
		setEFlag(EXTENDED);
		FeaturedMember fm;
		fm.m_pParent = new FFJSON(*pOrigParent, COPY_ALL, pObj);
		insertFeaturedMember(fm, FM_PARENT);
		if (orig.isEFlagSet(EXT_VIA_PARENT)) {
			map<string, int>* pOrigTabHead = orig.getFeaturedMember(FM_TABHEAD).
				tabHead;
			map<string, int>* pTabHead = new map<string, int>(*pOrigTabHead);
			setEFlag(EXT_VIA_PARENT);
			FeaturedMember fm;
			fm.tabHead = pTabHead;
			insertFeaturedMember(fm, FM_TABHEAD);
			if (isType(ARRAY)) {
				vector<FFJSON*>& vElems = *val.array;
				for (int i = 0; i < size; i++) {
					vElems[i]->setEFlag(EXT_VIA_PARENT);
					vElems[i]->insertFeaturedMember(fm, FM_TABHEAD);
				}
			} else if (isType(OBJECT)) {
				map<string, FFJSON*>::iterator itPairs = val.pairs->begin();
				while (itPairs != val.pairs->end()) {
					itPairs->second->setEFlag(EXT_VIA_PARENT);
					itPairs->second->insertFeaturedMember(fm, FM_TABHEAD);
					itPairs++;
				}
			}
		}
		Link* linkToParent = NULL;
		if (fm.m_pParent->isType(ARRAY) && fm.m_pParent->size == 1) {
			linkToParent = (*fm.m_pParent->val.array)[0]->getFeaturedMember(
				FM_LINK).link;
		} else if (fm.m_pParent->isType(OBJECT) && fm.m_pParent->size == 1) {
			linkToParent = (*fm.m_pParent->val.pairs)["*"]->getFeaturedMember(
				FM_LINK).link;
		} else {
			ffl_err(FFJ_MAIN, "Invalid parent size. Not 1.");
		}
		//set "this" as child to the parent
		Link& rLnParent = *linkToParent;
		vector<const string*> path;
		FFJSONPObj* pFPObjTemp = pObj;
		bool bParentFound = false;
		while (pFPObjTemp != NULL) {
			if (pFPObjTemp->value->isType(OBJECT)) {
				if (rLnParent.size() && pFPObjTemp->value->val.
					 pairs->find(rLnParent[0]) != pFPObjTemp->
					 value->val.pairs->end()) {
					bParentFound = true;
				}
				path.push_back(pFPObjTemp->name);
			} else if (pFPObjTemp->value->isType(ARRAY)) {
				try {
					if (pFPObjTemp->value->size > atoi(pFPObjTemp->name->c_str())) {
						bParentFound = true;
					}
					path.push_back(pFPObjTemp->name);
				} catch (Exception e) {
					ffl_err(FFJ_MAIN, "array member name is not a number");
				}
			}
			if (bParentFound) {
				FFJSON* pParentRoot = pFPObjTemp->value;
				int iParentLnIndexer = 0;
				do {
					if (pParentRoot->isType(OBJECT)) {
						pParentRoot = (*pParentRoot->val.pairs).
							find(rLnParent[iParentLnIndexer++])
							->second;
					} else if (pParentRoot->isType(ARRAY)) {
						try {
							pParentRoot = (*pParentRoot->val.array)
								[atoi(rLnParent[iParentLnIndexer++].c_str())];
						} catch (Exception e) {
							pParentRoot = NULL;
						}
					} else {
						pParentRoot = NULL;
					}
					if (iParentLnIndexer < rLnParent.size())
						pParentRoot = pObj->value->val.pairs->
							at(rLnParent[iParentLnIndexer++]);
				} while (pParentRoot && iParentLnIndexer <
							rLnParent.size());
				if (pParentRoot) {
					FFJSON* pffLink = new FFJSON();
					pffLink->setType(LINK);
					pffLink->val.fptr = this;
					FeaturedMember cFM;
					Link* pLnChild = new Link();
					for (int i = path.size() - 1; i >= 0; i--) {
						pLnChild->push_back(*path[i]);
					}
					cFM.link = pLnChild;
					pffLink->insertFeaturedMember(cFM, FM_LINK);
					if (!pParentRoot->isEFlagSet(HAS_CHILDREN)) {
						pParentRoot->setEFlag(HAS_CHILDREN);
						FeaturedMember fmChildren;
						fmChildren.m_pvChildren = new vector<FFJSON*>();
						pParentRoot->insertFeaturedMember(fmChildren,
																	 FM_CHILDREN);
					}
					vector<FFJSON*>* pvfChildren = pParentRoot->
						getFeaturedMember(FM_CHILDREN).m_pvChildren;
					pvfChildren->push_back(pffLink);
					break;
				} else {
					pFPObjTemp = pFPObjTemp->pObj;
				}
			} else {
				pFPObjTemp = pFPObjTemp->pObj;
			}
		}
	}
}

inline bool FFJSON::isWhiteSpace(char c) {
	switch (c) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return true;
		default:
			return false;
	}
}

inline bool FFJSON::isTerminatingChar(char c) {
	switch (c) {
		case ',':
		case '}':
		case ']':
			return true;
		default:
			return false;
	}
}

inline bool FFJSON::isInitializingChar(char c) {
	switch (c) {
		case ',':
		case '(':
		case '{':
		case '[':
		case ':':
		case ' ':
		case '\t':
			return true;
		default:
			return false;
	}
}

void FFJSON::init (
	const string& ffjson, int* ci, int indent, FFJSONPObj* pObj
) {
	if (pObj) {
		if (pObj->name) {
			if (!isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
				if (pObj->value->isType(OBJECT)) {
					pair<map<string, FFJSON*>::iterator, bool> prNew =
						pObj->value->val.pairs->insert(
							pair<string, FFJSON*>(*pObj->name, this)
						);
					if (pObj->value->size < MAX_ORDERED_MEMBERS) {
						pObj->m_pvpsMapSequence->push_back(prNew.first);
					} else if (pObj->m_pvpsMapSequence) {
						pObj->value->nullFeaturedMember(FM_MAP_SEQUENCE);
						pObj->m_pvpsMapSequence= nullptr;
					}
				} else if (pObj->value->isType(ARRAY)) {
					pObj->value->val.array->push_back(this);
				}
				++pObj->value->size;
			}
		} else {
			pObj=pObj->pObj;//it must be link
		}
	}
	int i = (ci == NULL) ? 0 : *ci;
	int j = ffjson.length();
	FeaturedMember fmMulLnBuf;
	fmMulLnBuf.m_psMultiLnBuffer= NULL;
	FFJSONPObj ffpo;
	while (i < j) {
		switch (ffjson[i]) {
			case '{': {
				++i;
				int objIdNail= i;
				string objId;
				int nind = getIndent(ffjson.c_str(), &i, indent);
				bool comment = false, gotoObjBackyard=false;
				FeaturedMember fmMapSequence;
				uint curlBegan=0, squareBegan=0;
				ffpo.value= this;
				ffpo.pObj= pObj;
				while (i < j) {
					switch (ffjson[i]) {
						case ':': {
							if (isType(UNDEFINED)) {
							  objtype:
								setType(OBJECT);
								fmMapSequence.m_pvpsMapSequence=
									new vector<map<string,FFJSON*>::iterator>();
								insertFeaturedMember(fmMapSequence,
															FM_MAP_SEQUENCE);
								ffpo.m_pvpsMapSequence=
									fmMapSequence.m_pvpsMapSequence;
								val.pairs= new map<string, FFJSON*>();
								if (gotoObjBackyard) {
									gotoObjBackyard=false;
									++i;
									goto ObjBackyard;
								}
							}
							objId = ffjson.substr(objIdNail, i - objIdNail);
							trimWhites(objId);
							trimQuotes(objId);
							ffpo.name= &objId;
							++i;
							FFJSON* obj= new FFJSON(ffjson, &i, nind, &ffpo);
							if (comment) {
								string comment("#");
								comment+= objId;
								if (val.pairs->find(comment) != val.pairs->end()) {
									obj->setEFlag(HAS_COMMENT);
								}
							}
							switch (objId[0]) {
								case '#':
									comment= true;
									obj->setEFlag(COMMENT);
									break;
								case '(':
									comment = false;
									if (objId[1] == 'T' && objId[2] == 'i' &&
										 objId[3] == 'm' && objId[4] == 'e'
										 && objId[5] == ')') {
										LinkNRef lnr = GetLinkString(pObj);
										string sLink = lnr.m_sLink +
											(*fmMapSequence.m_pvpsMapSequence)
											[size-2]->first;
										FFJSON* pOverLooker=
											MarkAsUpdatable(
												sLink, *(lnr.m_pRef?lnr.m_pRef:this));
										FeaturedMember fm;
										fm.m_pTimeStamp=obj->val.m_pFerryTimeStamp;
										pOverLooker->insertFeaturedMember(
											fm, FM_UPDATE_TIMESTAMP
										);
									}
								default:
									comment = false;
							}
							if (comment)--size;
							break;
						}
						case ',':
						settype:
							if (isType(UNDEFINED) && !curlBegan && !squareBegan) {
								setType(OBJ_TYPE::SET_TYPE);
								val.setPtr = new ffset();
							}
							if (isType(OBJ_TYPE::SET_TYPE)) {
								string name = to_string(size);
								ffpo.name=&name;
								FFJSON* obj =
									new FFJSON(ffjson, &objIdNail, nind,
												  &ffpo);
								if (!obj->isType(UNDEFINED) && !obj->isType(NUL)) { 
									pair<ffset::iterator,bool> ret =
										val.setPtr->insert(obj);
									if (!ret.second) {
										--size;
										delete obj;
									}
								} else {
									--size;
									delete obj;
									if (!size) {
										delete val.setPtr;
										goto objtype;
									}
									gotoObjBackyard=true;
								}
							}
							++i;
							if (gotoObjBackyard) {
								gotoObjBackyard=false;
								goto ObjBackyard;
							}
							objIdNail = i;
							break;
						case '{':
							++curlBegan;
							++i;
							break;
						case '}':
							if (!curlBegan && !squareBegan) {
								if (isType(UNDEFINED)||isType(SET_TYPE)) {
									gotoObjBackyard = true;
									goto settype;
								}
								++i;
								goto ObjBackyard;
							} else {
								--curlBegan;
							}
							++i;
							break;
						case '[':
							++squareBegan;
							++i;
							break;
						case ']':
							--squareBegan;
							++i;
							break;
						default:
							++i;
					}
				}
			  ObjBackyard:
				goto backyard;
			}
			case '[': {
				setType(ARRAY);
				size = 0;
				val.array = new vector<FFJSON*>();
				++i;
				int objNail = i;
				int nind = getIndent(ffjson.c_str(), &i, indent);
				ffpo.value = this;
				ffpo.pObj = pObj;
				FFJSON* obj = nullptr;
				while (i < j) {
					string index = to_string(val.array->size());
					ffpo.name = &index;
					obj = new FFJSON(ffjson, &i, nind, &ffpo);
					if (obj->isType(NUL) && ffjson[i] == ']' && size == 1) {
						delete obj;
						obj = nullptr;
						val.array->pop_back();
						--size;
					} else if ((obj->isType(NUL) || obj->isType(UNDEFINED)) &&
								  obj->isQType(NONE) && !obj->isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
						delete obj;
						obj = nullptr;
						val.array->pop_back();
						val.array->push_back(NULL);
					}
					bool bLastObjIsMulLnStr = obj && obj->isType(STRING) &&
						m_uFM.m_bIsMultiLineArray &&
						(ffjson[i] == '\t' || ffjson[i] == '\n' ||
						 ffjson[i] == '\r');
					while (ffjson[i] == ' ' && ffjson[i] == '\t')i++;
					bool bEndOfMulLnStrArr =
						(m_uFM.m_bIsMultiLineArray &&
						 (ffjson[i] == '\n' || ffjson[i] == '\r'));
					if (!bLastObjIsMulLnStr && !bEndOfMulLnStrArr) {
						while (ffjson[i] != ',' && ffjson[i] != ']' && i < j) {
							++i;
						}
					}
					if (bEndOfMulLnStrArr) {
						ReadMultiLinesInContainers(ffjson, i, ffpo);
						m_uFM.m_bIsMultiLineArray = false;
					}
					if (ffjson[i] == ',' || (bLastObjIsMulLnStr &&
													 !bEndOfMulLnStrArr)) {
						++i;
						objNail = i;
					} else if (ffjson[i] == ']') {
						++i;
						break;
					} else {
						++i;
					}
				}
				goto backyard;
			}
			case '"': {
				++i;
				setType(STRING);
				val.string = NULL;
				int nind = getIndent(ffjson.c_str(), &i, indent);
				vector<char*> bufvec;
				int k = 0;
				int pi = 0;
				char* buf = NULL;
				bool bMultiLineTxt = false;
				bool bMulLnBuf = false;
				while (i < j) {
					if ((k % 100) == 0 && (pi == 0 || i >= 100 + pi)) {
						pi = i;
						buf = new char[100];
						bufvec.push_back(buf);
						size += 100;
						k = 0;
					}
					if (ffjson[i] == '\\' && nind == indent) {
						++i;
						switch (ffjson[i]) {
							case 'n':
								buf[k] = '\n';
								break;
							case 'r':
								buf[k] = '\r';
								break;
							case 't':
								buf[k] = '\t';
								break;
							case '\\':
								buf[k] = '\\';
								break;
							default:
								buf[k] = ffjson[i];
								break;
						}
						++i;
						++k;
					} else if (ffjson[i] == '\n') {
						if (ffjson[i - 1] == '"')bMultiLineTxt = true;
						if (!bMultiLineTxt) {
							buf[k] = '\n';
							buf[k + 1] = '\0';
							pObj->value->m_uFM.m_bIsMultiLineArray = true;
							bMulLnBuf = true;
							setEFlag(STRING_INIT);
							fmMulLnBuf.m_psMultiLnBuffer = new string(buf);
							this->insertFeaturedMember(fmMulLnBuf, FM_MULTI_LN);
							break;
						}
						int ind = ++i;
						while (ffjson[ind] == '\t' && (ind - i) < nind) {
							++ind;
						}
						if ((ind - i) == nind) {
							i += nind;
						} else if (ffjson[ind] == '"' && ((ind - i) ==
																	 (nind - 1))) {
							i = ind + 1;
							break;
						}
						if (k != 0) {
							buf[k] = '\n';
							++k;
						}
					} else if (ffjson[i] == '\r' && ffjson[i + 1] == '\n') {
						if (ffjson[i - 1] == '"')bMultiLineTxt = true;
						if (!bMultiLineTxt) {
							buf[k] = '\n';
							buf[k + 1] = '\0';
							pObj->value->m_uFM.m_bIsMultiLineArray = true;
							bMulLnBuf = true;
							setEFlag(STRING_INIT);
							fmMulLnBuf.m_psMultiLnBuffer = new string(buf);
							this->insertFeaturedMember(fmMulLnBuf, FM_MULTI_LN);
							break;
						}
						++i;
						int ind = ++i;
						while (ffjson[ind] == '\t' && (ind - i) < nind) {
							++ind;
						}
						if ((ind - i) == nind) {
							i += nind;
						} else if (ffjson[ind] == '"' && ((ind - i) ==
																	 (nind - 1))) {
							i = ind + 1;
							break;
						}
						if (k != 0) {
							buf[k] = '\n';
							++k;
						}
					} else if (ffjson[i] == '\t' && !bMultiLineTxt) {
						buf[k] = '\n';
						buf[k + 1] = '\0';
						pObj->value->m_uFM.m_bIsMultiLineArray = true;
						bMulLnBuf = true;
						setEFlag(STRING_INIT);
						fmMulLnBuf.m_psMultiLnBuffer = new string(buf);
						this->insertFeaturedMember(fmMulLnBuf, FM_MULTI_LN);
						break;
					} else if (ffjson[i] == '"' && nind == indent) {
						++i;
						break;
					} else {
						buf[k] = ffjson[i];
						++k;
						++i;
					}
				}
				buf[k] = '\0';
				int ii = 0;
				size += k - 100;
				val.string = new char[size + 1];
				int iis = bufvec.size() - 1;
				while (ii < iis) {
					memcpy(val.string + (100 * ii), bufvec[ii], 100);
					delete[] bufvec[ii];
					++ii;
				}
				memcpy(val.string + (100 * ii), bufvec[ii], k + 1);
				delete[] bufvec[ii];
				char* pNewLnPos = val.string;
				char* pOldNewLnPos = NULL;
				FeaturedMember fmWidth;
				fmWidth.width = 0;
				while (*pNewLnPos) {
					pOldNewLnPos = pNewLnPos;
					pNewLnPos = strchr(pNewLnPos, '\n');
					if (!pNewLnPos) {
						// Take care of size when implementing UTF-8
						pNewLnPos = val.string + size;
						if (!bMulLnBuf) {
							if (pNewLnPos - pOldNewLnPos > fmWidth.width) {
								setEFlag(LONG_LAST_LN);
								fmWidth.width = pNewLnPos - pOldNewLnPos;
							} else if (pNewLnPos - pOldNewLnPos == fmWidth.width
										  - 1)
								setEFlag(ONE_SHORT_LAST_LN);
						}
						break;
					} else {
						pNewLnPos++;
					}
					if (pNewLnPos - pOldNewLnPos - 1 > fmWidth.width)
						fmWidth.width = pNewLnPos - pOldNewLnPos - 1;
				}
				insertFeaturedMember(fmWidth, FM_WIDTH);
				goto backyard;
			}
			case '<': {
				++i;
				int xmlNail = i;
				string xmlTag;
				int length = -1;
				bool tagset = false;
				while (ffjson[i] != '>' && i < j) {
					if (ffjson[i] == ' ') {
						tagset = true;
						if (ffjson[i + 1] == 'l' &&
							 ffjson[i + 2] == 'e' &&
							 ffjson[i + 3] == 'n' &&
							 ffjson[i + 4] == 'g' &&
							 ffjson[i + 5] == 't' &&
							 ffjson[i + 6] == 'h') {
							i += 7;
							while (ffjson[i] != '=' && i < j) {
								++i;
							}
							++i;
							while (ffjson[i] != '"' && i < j) {
								++i;
							}
							++i;
							string lengthstr;
							while (ffjson[i] != '"' && i < j) {
								lengthstr += ffjson[i];
								++i;
							}
							length = atoi(lengthstr.c_str());
						}
					} else if (!tagset) {
						xmlTag += ffjson[i];
					}
					++i;
				}
				val.string = NULL;
				setType(XML);
				++i;
				xmlNail = i;
				if (length>-1 && length < (j - i)) {
					i += length;
				}
				while (i < j) {
					if (ffjson[i] == '<' &&
						 ffjson[i + 1] == '/') {
						if (xmlTag.compare(ffjson.substr(i + 2, xmlTag.length()))
							 == 0 && ffjson[i + 2 + xmlTag.length()] == '>') {
							size = i - xmlNail;
							val.string = new char[size];
							memcpy(val.string, ffjson.c_str() + xmlNail,
									 size);
							i += 3 + xmlTag.length();
							break;
						}
					}
					++i;
				}
				if (val.string == NULL)setType(NUL);
				goto backyard;
			}
			case '(': {
				++i;
				int typeNail = i;
				while (ffjson[i] != ')' && i < j) {
					++i;
				}
				size = atoi(ffjson.c_str()+typeNail);
				val.vptr = (uint8_t*)malloc(size*sizeof(uint8_t));
				++i;
				memcpy(val.vptr, ffjson.c_str() + i, size);
				setType(FFJSON::BINARY);
				i += size;
				goto backyard;
			}
			case 't': {
				if (ffjson[i + 1] == 'r' &&
					 ffjson[i + 2] == 'u' &&
					 ffjson[i + 3] == 'e') {
					setType(BOOL);
					val.boolean = true;
					i += 4;
					goto backyard;
				}
				break;
			}
			case 'f': {
				if (ffjson[i + 1] == 'a' &&
					 ffjson[i + 2] == 'l' &&
					 ffjson[i + 3] == 's' &&
					 ffjson[i + 4] == 'e') {
					setType(BOOL);
					val.boolean = false;
					i += 5;
					goto backyard;
				}
				break;
			}
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
			case '+':
				if(!isInitializingChar(ffjson[i-1]))break;{
					int numNail = i;
					++i;
					int precision = 0;
					while ((ffjson[i] >= '0' && ffjson[i] <= '9') ||
							 (ffjson[i] == '.' && (ffjson[i + 1] >= '0' &&
														  ffjson[i + 1] <= '9'))) {
						if (ffjson[i]=='.') {
							precision = i - numNail + 1;
						}
						++i;
					}
					size = i - numNail;
					if (!precision)
						precision= size;
					string num = ffjson.substr(numNail, i - numNail);
					if (num.length() == 20) {
						size= sizeof(FerryTimeStamp);
						val.m_pFerryTimeStamp= (FerryTimeStamp*)malloc(size);
						*val.m_pFerryTimeStamp= FerryTimeStamp(num);
						setType(OBJ_TYPE::BINARY);
						goto backyard;
					}
					size_t s = 0;
					val.number = stod(num, &s);
					FeaturedMember cFM;
					cFM.precision = size-precision;
					if (cFM.precision) {
						setEFlag(PRECISION);
						insertFeaturedMember(cFM, FM_PRECISION);
					}
					setType(OBJ_TYPE::NUMBER);
					goto backyard;
				}
			case ':': {
				if (ffjson[i + 1] == '/'
					 && ffjson[i + 2] == '/') {
					if (
						ffjson[i - 1] == 'e' && ffjson[i - 2] == 'l' &&
						ffjson[i - 3] == 'i' && ffjson[i - 4] == 'f'
						&& (i <= 5 || (ffjson[i - 5] < 'a' || ffjson[i - 5] > 'z'))
					) {
						i+= 3;
						string path;
						string objCaster;
						bool objCastNail= false;
						int k = 0;
						while (i < j && !isTerminatingChar(ffjson[i])) {
							if (!isWhiteSpace(ffjson[i])) {
								if (ffjson[i] == '|') {
									objCastNail= true;
									k= 0;
								} else if (objCastNail) {
									objCaster+= ffjson[i];
									++k;
								} else {
									path+= ffjson[i];
									++k;
								}
							}
							++i;
						}
						if (path.length() > 0) {
							if (path[0]!='/') {
								FFJSONPObj* lpobj= pObj;
								while (lpobj) {
									if (lpobj->value->isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
										const char* pfn= lpobj->value->
											getFeaturedMember(FM_FILE).m_sFileName;
										int pfnl= strlen(pfn)-1;
										while (pfn[pfnl]!='/' && pfnl>=0) {
											--pfnl;
										}
										path.insert(0, pfn, pfnl+1);
										path.insert(pfnl+1, "./");
										break;
									}
									lpobj= lpobj->pObj;
								}
							}
							ifstream ifs(path.c_str(), ios::in | ios::ate);
							setEFlag(FILE);
							FeaturedMember fm;
							fm.m_sFileName= new char[path.length()+1];
							strcpy(fm.m_sFileName, path.c_str());
							insertFeaturedMember(fm, FM_FILE);
							if (ifs.is_open()) {
								string ffjsonStr;
								ifs.seekg(0, ios::end);
								uint8_t t= UNDEFINED;
								if (objCaster.length() > 0) {
									t= STR_OBJ[objCaster];
									if (t == STRING || t == OBJECT || t == ARRAY) {
										ffjsonStr.reserve((int) ifs.tellg() + 2);
										if (t == STRING) {
											ffjsonStr += "\"\n";
										} else if (t == OBJECT) {
											ffjsonStr += "{\n";
										} else if (t == ARRAY) {
											ffjsonStr += "[\n";
										} else {
											t= UNDEFINED;
										}
										setEFlag(CASTFILE);
									};
								} else {
									ffjsonStr.reserve(ifs.tellg());
								}
								ifs.seekg(0, ios::beg);
								ffjsonStr.append((istreambuf_iterator<char>(ifs)),
													  istreambuf_iterator<char>());
								ifs.close();
								switch (t) {
									case STRING:
										ffjsonStr+='"';
										break;
									case OBJECT:
										ffjsonStr+='}';
										break;
									case ARRAY:
										ffjsonStr+=']';
										break;
									default:
										break;
								}
								init(ffjsonStr, NULL, 0, pObj);
							} else {
								ffl_warn(FFJ_MAIN, "Couldn't open %s", path.c_str());
								ifs.close();
							}
						}
					} else {
						while (i < j&&!isTerminatingChar(ffjson[i])) {
							++i;
						}
					}
				}
				goto backyard;
			}
			case '?':
				setQType(QUERY);
				++i;
				goto backyard;
			case '^':
				setQType(UPDATE);
				++i;
				goto backyard;
			case 'd': {
				if (ffjson[i + 1] == 'e' &&
					 ffjson[i + 2] == 'l' &&
					 ffjson[i + 3] == 'e' &&
					 ffjson[i + 4] == 't' &&
					 ffjson[i + 5] == 'e') {
					setQType(DEL);
					i += 6;
					goto backyard;
				}
				break;
			}
			case '!': {
				setType(NUL);
				pObj->value->setQType(NQUERY);
				goto backyard;
			}
			case 'n': {
				if (ffjson[i + 1] == 'u' &&
					 ffjson[i + 2] == 'l' &&
					 ffjson[i + 3] == 'l') {
					setType(NUL);
					i += 4;
					goto backyard;
				}
				break;
			}
			case ',':
			case '}':
			case ']': {
				// NULL Objects or links caught here eg. "[]", ",,", ",]", "name:,}"
				string subffj(ffjson.c_str() + *ci, i - *ci);
				trimWhites(subffj);
				if (subffj.length() > 0) {
					vector<string>* prop = new vector<string>();
					explode(".", subffj, *prop);
					FFJSON* obj = returnNameIfDeclared(*prop, pObj);
					if (!obj) {
						if (!pObj) {
							delete prop;
							setType(UNDEFINED);
							goto backyard;
						}
						int pL = 0;
						FFJSONPObj* lfpo = pObj;
						while (pL<prop->size() && !(*prop)[pL].size()) {
							lfpo=lfpo->pObj;
							if (!lfpo) {
								delete prop;
								setType(UNDEFINED);
								goto backyard;
							}
							++pL;
						}
						SymlinkTrail sT;
						sT.l=prop;
						sT.ln=this;
						lfpo->symTrVec.push_back(sT);
					}
					setType(LINK);
					val.fptr = obj;
					FeaturedMember cFM;
					cFM.link = prop;
					insertFeaturedMember(cFM, FM_LINK);
				} else {
					setType(NUL);
					size = 0;
				}
				goto backyard;
			}
		}
		++i;
	}
  backyard:
	if (!isType(UNDEFINED) &&
		 !(isType(STRING) && fmMulLnBuf.m_psMultiLnBuffer)) {
		while ((ffjson[i] == ' ' || ffjson[i] == '\t') && i < j) {
			++i;
		}
		if (ffjson[i] == '|') {
			++i;
			ffpo.name=NULL;
			ffpo.pObj=pObj;
			ffpo.value=this;
			ffpo.m_pvpsMapSequence=NULL;
			FFJSON* obj = new FFJSON(ffjson, &i, indent, &ffpo);
			if (inherit(*obj, &ffpo)) {

			} else {
				delete obj;
			}
		}
	} /*else if (isType(UNDEFINED)) {
		 int k=*ci;
		 while ((ffjson[k] == ' ' || ffjson[k] == '\t') && k < j) ++k;
		 ffpo.value = this;
		 ffpo.pObj = pObj;
		 FFJSONPObj* pPObj = pObj;
		 FFJSON* link=nullptr;
		 Iterator it;
		 string key;
		 bool done=false;
		 while(true) {
		 switch(ffjson[k]) {
		 case '.':
		 if (ffjson[k+1] == '.') {
		 pPObj=pPObj->pObj;
		 k+=2;
		 }
		 break;
		 case '/':
		 ++k;
		 goto gotkey;
		 break;
		 case ' ':
		 case '\n':
		 case '\t':
		 case ',':
		 case ']':
		 case '}':
		 done=true;
		 gotkey:
		 if(!key.length()){link=pPObj->value;break;}
		 if(link){
		 if(link->isType(OBJECT)||link->isType(ARRAY))
		 link=&(*link)[key];
		 }else{
		 ffl_warn(FFJ_MAIN, "%s not found in %s at %d",
		 key.c_str(),GetLinkString(pObj).m_sLink,k);
		 break;
		 }
		 it = pPObj->value->find(key);
		 if(it!=pPObj->value->end()) {
		 link=&(*pPObj->value)[key];
		 }
		 key="";
		 break;
		 default:
		 key+=ffjson[k];
		 ++k;
		 break;
		 }
		 if(done)break;
		 }
		 if(!link)goto done;
		 setType(LINK);
		 val.fptr=link;
		 done:
		 ;
		 }*/
	for (int j=ffpo.symTrVec.size()-1;j>=0;--j) {
		FFJSON* tln=this;
		for (int i=0;i<ffpo.symTrVec[j].l->size();++i) {
			if (!(*ffpo.symTrVec[j].l)[i].size()) {
				continue;
			}
			FFJSON::Iterator it = tln->find((*ffpo.symTrVec[j].l)[i]);
			if (it==tln->end()) {
				tln=nullptr;
				break;
			} else {
				tln = &*it;
			}
		}
		if (tln) {
			ffpo.symTrVec[j].ln->val.fptr=tln;
		} else {
			ffpo.symTrVec[j].ln->freeObj();
		}
	}
	if (ci != NULL)*ci = i;
}

void FFJSON::ReadMultiLinesInContainers(const string& ffjson, int& i,
													 FFJSONPObj & pObj) {
	if (ffjson[i] == '\n' || ffjson[i] == '\r') {
		if (ffjson[i] == '\r')i++;
		int iI = 0;
		int iFFJSONSize = ffjson.length();
		int iArrSize = pObj.value->val.array->size();
		while (iI < iArrSize) {
			while (ffjson[i] == ' ' || ffjson[i] == '\t')i++;
			if (ffjson[i] == '\n' || ffjson[i] == '\r') {
				if (ffjson[i] == '\r')i++;
				i++;
				iI = 0;
				continue;
			}
			FeaturedMember fmMulLnBuf = (*pObj.value->val.array)[iI]->
				getFeaturedMember(FM_MULTI_LN);
			while (!((*pObj.value->val.array)[iI]->isType(STRING) &&
						fmMulLnBuf.m_psMultiLnBuffer != NULL)) {
				iI++;
				if (iI >= iArrSize)break;
				fmMulLnBuf = (*pObj.value->val.array)[iI]->
					getFeaturedMember(FM_MULTI_LN);
			}
			if (iI == iArrSize)break;
			string& sTemp = *fmMulLnBuf.m_psMultiLnBuffer;
			bool bBreak = false;
			while (!bBreak && i < iFFJSONSize) {
				switch (ffjson[i]) {
					case '\\':
						i++;
						switch (ffjson[i]) {
							case 'n':
								sTemp += '\n';
								break;
							case 'r':
								sTemp += '\r';
								break;
							case 't':
								sTemp += '\t';
								break;
							case '\\':
								sTemp += '\\';
								break;
							default:
								sTemp += ffjson[i];
								break;
						}
						i++;
					case '\r':
						i++;
					case '\n':
						iI = 0;
						bBreak = true;
						sTemp += '\n';
						i++;
						break;
					case '\t':
						iI++;
						bBreak = true;
						sTemp += '\n';
						i++;
						break;
					case '"':
						*(*pObj.value->val.array)[iI] = sTemp;
						//(*pObj.value->val.array)[iI]->
						//deleteFeaturedMember(FM_MULTI_LN);
						delete &sTemp;
						if (iI < iArrSize - 1) while (ffjson[i] != ',')i++;
						i++;
						bBreak = true;
						iI++;
						break;
					default:
						sTemp += ffjson[i];
						i++;
						break;
				}
			}
		}
	}
}

void FFJSON::setFMCount(uint32_t iFMCount) {
	iFMCount <<= 28;
	flags &= 0x0FFFFFFF;
	flags |= iFMCount;

}

void FFJSON::insertFeaturedMember (FeaturedMember& fms, FeaturedMemType fMT) {
	FeaturedMember* pFMS = &m_uFM;
	uint32_t iFMCount = flags >> 28;
	uint32_t iFMTraversed = 0;
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fMT == FM_MULTI_LN) {
			if (pFMS->m_psMultiLnBuffer == NULL) {
				pFMS->m_psMultiLnBuffer = fms.m_psMultiLnBuffer;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_psMultiLnBuffer = fms.m_psMultiLnBuffer;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_psMultiLnBuffer = pFMS->m_psMultiLnBuffer;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(STRING)) {
		if (fMT == FM_WIDTH) {
			if (pFMS->width == 0) {
				pFMS->width = fms.width;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.width = fms.width;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.width = pFMS->width;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(OBJECT)) {
		if (fMT == FM_MAP_SEQUENCE) {
			if (pFMS->m_pvpsMapSequence == NULL) {
				pFMS->m_pvpsMapSequence = fms.m_pvpsMapSequence;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_pvpsMapSequence = fms.m_pvpsMapSequence;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_pvpsMapSequence = pFMS->m_pvpsMapSequence;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (fMT == FM_TABHEAD) {
			if (pFMS->tabHead == NULL) {
				pFMS->tabHead = fms.tabHead;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.tabHead = fms.tabHead;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.tabHead = pFMS->tabHead;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isType(LINK)) {
		if (fMT == FM_LINK) {
			if (pFMS->link == NULL) {
				pFMS->link = fms.link;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.link = fms.link;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.link = pFMS->link;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXTENDED) && (isType(OBJECT) || isType(ARRAY))) {
		if (fMT == FM_PARENT) {
			if (pFMS->m_pParent == NULL) {
				pFMS->m_pParent = fms.m_pParent;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_pParent = fms.m_pParent;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_pParent = pFMS->m_pParent;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJECT) || isType(ARRAY))) {
		if (fMT == FM_CHILDREN) {
			if (pFMS->m_pvChildren == NULL) {
				pFMS->m_pvChildren = fms.m_pvChildren;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_pvChildren = fms.m_pvChildren;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_pvChildren = pFMS->m_pvChildren;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isQType(UPDATE)) {
		if (fMT == FM_UPDATE_TIMESTAMP) {
			if (pFMS->m_pTimeStamp == NULL) {
				pFMS->m_pTimeStamp = fms.m_pTimeStamp;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_pTimeStamp = fms.m_pTimeStamp;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_pTimeStamp = pFMS->m_pTimeStamp;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fMT == FM_FILE) {
			if (!pFMS->m_sFileName) {
				pFMS->m_sFileName = fms.m_sFileName;
			} else {
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_pFMH.m_pFMH = pFMS->m_pFMH;
				pNewFMH->m_uFM.m_sFileName = fms.m_sFileName;
				pFMS->m_pFMH = pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else {
			if (iFMCount - iFMTraversed == 1) {
				//Should insert New FM hook before the right FMType match
				FeaturedMemHook* pNewFMH = new FeaturedMemHook();
				pNewFMH->m_uFM.m_sFileName = pFMS->m_sFileName;
				pFMS->m_pFMH = pNewFMH;
				pFMS = &pNewFMH->m_pFMH;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
}

FFJSON::FeaturedMember FFJSON::getFeaturedMember (FeaturedMemType fMT) const {
	const FeaturedMember* pFMS = &m_uFM;
	uint32_t iFMCount = flags >> 28;
	uint32_t iFMTraversed = 0;
	FeaturedMember decoyFM;
	decoyFM.precision = 0;
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fMT == FM_MULTI_LN) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(STRING)) {
		if (fMT == FM_WIDTH) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(OBJECT) && size<MAX_ORDERED_MEMBERS) {
		if (fMT == FM_MAP_SEQUENCE) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (fMT == FM_TABHEAD) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(LINK)) {
		if (fMT == FM_LINK) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet(EXTENDED) && (isType(OBJECT) || isType(ARRAY))) {
		if (fMT == FM_PARENT) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJECT) || isType(ARRAY))) {
		if (fMT == FM_CHILDREN) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isQType(UPDATE)) {
		if (fMT == FM_UPDATE_TIMESTAMP) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fMT == FM_FILE) {
			if (iFMCount - iFMTraversed == 1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount - iFMTraversed == 1) {
				return decoyFM;
			} else {
				pFMS = &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	return decoyFM;
}

void DeleteChildLinks(vector<FFJSON*>* childLinks) {
	for (int i = 0; i < childLinks->size(); i++) {
		delete (*childLinks)[i];
	}
}

void FFJSON::destroyAllFeaturedMembers (bool bExemptQueries) {
	uint32_t iFMCount = flags >> 28;
	if (!iFMCount)
		return;
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (iFMCount == 1) {
			//Deletion of the multilinebuf is responsibility of me
			//delete fmHolder.m_psMultiLnBuffer;
		} else {
			//Deletion of the multilinebuf is responsibility of me
			//delete fmHolder.m_pFMH->m_uFM.m_psMultiLnBuffer;
			FeaturedMemHook* pFMHHolder= m_uFM.m_pFMH;
			m_uFM= pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
		clearEFlag(STRING_INIT);
	}
	if (isType(STRING)) {
		if (iFMCount == 1) {
			m_uFM.width = 0;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			pFMHHolder->m_uFM.width = 0;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
		setType(UNDEFINED);
	}
	if (isType(OBJECT) && size<MAX_ORDERED_MEMBERS) {
		if (iFMCount == 1) {
			delete m_uFM.m_pvpsMapSequence;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			delete pFMHHolder->m_uFM.m_pvpsMapSequence;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (iFMCount == 1) {
			if (isEFlagSet(EXTENDED) && getType() != NUMBER) {
				delete m_uFM.tabHead;
			}
		} else {
			if (isEFlagSet(EXTENDED) && getType() != NUMBER) {
				delete m_uFM.m_pFMH->m_uFM.tabHead;
			}
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isType(LINK)) {
		if (iFMCount == 1) {
			delete m_uFM.link;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			delete pFMHHolder->m_uFM.tabHead;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isEFlagSet(EXTENDED) && (isType(OBJECT) || isType(ARRAY))) {
		if (iFMCount == 1) {
			delete m_uFM.m_pParent;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			delete pFMHHolder->m_uFM.m_pParent;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isEFlagSet(HAS_CHILDREN) && (isType(OBJECT) || isType(ARRAY))) {
		if (iFMCount == 1) {
			DeleteChildLinks(m_uFM.m_pvChildren);
			delete m_uFM.m_pvChildren;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			DeleteChildLinks(pFMHHolder->m_uFM.m_pvChildren);
			delete pFMHHolder->m_uFM.m_pvChildren;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isQType(UPDATE) && !bExemptQueries) {
		if (iFMCount == 1) {
			m_uFM.m_pTimeStamp=NULL;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			pFMHHolder->m_uFM.m_pTimeStamp=NULL;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
	}
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (iFMCount == 1) {
			delete[] m_uFM.m_sFileName;
		} else {
			FeaturedMemHook* pFMHHolder = m_uFM.m_pFMH;
			delete[] pFMHHolder->m_uFM.m_sFileName;
			m_uFM = pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		iFMCount--;
		clearEFlag(FILE);
	}
	setFMCount(iFMCount);
}

void FFJSON::nullFeaturedMember (FeaturedMemType fmt) {
	FeaturedMember* pFMS= &m_uFM;
	uint32_t iFMCount= flags >> 28;
	uint32_t iFMTraversed= 0;
	if (isType(OBJECT) && fmt==FM_MAP_SEQUENCE && size==MAX_ORDERED_MEMBERS) {
		if (iFMCount==1) {
			delete m_uFM.m_pvpsMapSequence;
			m_uFM.m_pvpsMapSequence= NULL;
		} else {
			FeaturedMemHook* pFMHHolder= m_uFM.m_pFMH;
			delete pFMHHolder->m_uFM.m_pvpsMapSequence;
			m_uFM= pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		--iFMCount;
		setFMCount(iFMCount);
	}
}
void FFJSON::deleteFeaturedMember (FeaturedMemType fmt) {
	FeaturedMember* pFMS = &m_uFM;
	uint32_t iFMCount = flags >> 28;
	uint32_t iFMTraversed = 0;
	FeaturedMember* pfmPre = NULL;
	if (this->isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fmt == FM_MULTI_LN) {
			if (iFMCount - iFMTraversed == 1) {
				delete pFMS->m_psMultiLnBuffer;
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.m_psMultiLnBuffer;
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
			clearEFlag(STRING_INIT);
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
	if (this->isType(STRING)) {
		if (fmt == FM_WIDTH) {
			// can't be deleted as every object should contain MapSequence
			return;
			if (iFMCount - iFMTraversed == 1) {
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (isType(OBJECT) && fmt==FM_MAP_SEQUENCE) {
		if (iFMCount == 1) {
			delete m_uFM.m_pvpsMapSequence;
			m_uFM.m_pvpsMapSequence= NULL;
		} else {
			FeaturedMemHook* pFMHHolder= m_uFM.m_pFMH;
			delete pFMHHolder->m_uFM.m_pvpsMapSequence;
			m_uFM= pFMHHolder->m_pFMH;
			delete pFMHHolder;
		}
		--iFMCount;
		setFMCount(iFMCount);
	}
	if (this->isEFlagSet(EXT_VIA_PARENT)) {
		if (fmt == FM_TABHEAD) {
			return; // Inheritance can't be removed. Will think of it later.
			if (iFMCount - iFMTraversed == 1) {
				if (this->isEFlagSet(EXTENDED) && getType() != NUMBER) {
					delete pFMS->tabHead;
				}
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				if (this->isEFlagSet(EXTENDED) && getType() != NUMBER) {
					delete pFMS->m_pFMH->m_uFM.tabHead;
				}
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
	if (this->isType(LINK)) {
		if (fmt == FM_LINK) {
			return; //A link should have a Link
			if (iFMCount - iFMTraversed == 1) {
				delete pFMS->link;
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.tabHead;
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXTENDED)) {
		if (fmt == FM_PARENT) {
			// Inheritance can't be removed. will think of it later.
			if (iFMCount - iFMTraversed == 1) {
				delete pFMS->m_pParent;
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.m_pParent;
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJECT) || isType(ARRAY))) {
		if (fmt == FM_CHILDREN) {
			return; //Inheritance can't be remove. will think of it later
			if (iFMCount - iFMTraversed == 1) {
				DeleteChildLinks(pFMS->m_pvChildren);
				delete pFMS->m_pvChildren;
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				DeleteChildLinks(pFMS->m_pFMH->m_uFM.m_pvChildren);
				delete pFMS->m_pFMH->m_uFM.m_pvChildren;
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fmt == (FeaturedMemType)FILE) {
			if (iFMCount - iFMTraversed == 1) {
				delete[] pFMS->m_sFileName;
				if (pfmPre) {
					*pfmPre = pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete[] pFMS->m_pFMH->m_uFM.m_sFileName;
				FeaturedMember* pTempFMS = &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS = *pTempFMS;
			}
		}
		pfmPre = pFMS;
		++iFMTraversed;
	}
}

FFJSON* FFJSON::returnNameIfDeclared (vector<string>& prop,
												  FFJSON::FFJSONPObj* fpo) const {
	int j = 0;
	int lp=0;
	if (fpo==nullptr){
		fpo=new FFJSON::FFJSONPObj();
		fpo->value=const_cast<FFJSON*>(this);
		for (int i=0; i<prop.size();++i) {
			if (!prop[i].size()) {
				++lp;
			} else {
				break;
			}
		}
	}
	while (fpo != nullptr) {
		FFJSON* fp = fpo->value;
		j = lp;
		while (j < prop.size()) {
			if (!prop[lp].size()) {
				++lp;
				break;
			}
			if (fp->isType(OBJECT)) {
				if (fp->val.pairs->find(prop[j]) != fp->val.pairs->end()) {
					fp = (*fp->val.pairs)[prop[j]];
				} else {
					break;
				}
			} else if (fp->isType(ARRAY)) {
				int index = -1;
				index = atoi(prop[j].c_str());
				if (index==0 && prop[j][0]!='0') {
					break;
				}
				if (index < fp->size) {
					fp = (*fp->val.array)[index];
				}
			} else if (fp->isType(LINK)) {
				fp = fp->val.fptr;
			} else {
				break;
			}
			++j;
			if (j == prop.size())return fp;
		}
		if (fpo->value==this) {
			delete fpo;
			return nullptr;
		}
		fpo = fpo->pObj;
	}
	return nullptr;
}

FFJSON* FFJSON::markTheNameIfExtended(FFJSONPrettyPrintPObj * fpo) {
	map<string, FFJSON*>::iterator it = val.pairs->begin();
	FFJSONPrettyPrintPObj* oFPO = fpo;
	while (it != val.pairs->end()) {
		if (it->second->isEFlagSet(EXTENDED) && !it->second->isType(STRING)) {
			FFJSON* pParent = it->second->getFeaturedMember(FM_PARENT).
				m_pParent;
			if (pParent->isType(ARRAY)) {
				pParent = (*pParent->val.array)[0];
			} else if (pParent->isType(OBJECT)) {
				pParent = (*pParent->val.pairs)["*"];
			}
			const vector<string>& prop = *pParent->getFeaturedMember(FM_LINK).
				link;
			int j = 0;
			const string* pChildRootKey = &it->first;
			fpo = oFPO;
			while (fpo != NULL) {
				FFJSON* fp = fpo->value;
				FFJSON* pLinkRoot = NULL;
				const string* pChildName = NULL;
				const string* pParentName = NULL;
				j = 0;
				while (j < prop.size()) {
					if (fp->isType(OBJECT)) {
						map<string, FFJSON*>::iterator itKeyValue = fp->val.
							pairs->find(prop[j]);
						if (itKeyValue != fp->val.pairs->end()) {
							if (pLinkRoot == NULL) {
								pLinkRoot = fp;
								pChildName = pChildRootKey;
								pParentName = &itKeyValue->first;
							}
							fp = itKeyValue->second;
							j++;
							if (j == prop.size()) {
								(*fpo->m_mpDeps)[pChildName] = pParentName;
							}
						} else {
							break;
						}
					} else if (fp->isType(ARRAY)) {
						int index = -1;
						index = atoi(prop[j].c_str());
						if (index==0 && prop[j][0]!='0') {
							break;
						}
						if (index < fp->size) {
							fp = (*fp->val.array)[index];
							j++;
							if (j == prop.size()) {
								//array can't be re-arranged; marking is not 
								//necessary.
							}
						} else {
							break;
						}
					} else if (fp->isLink()) {
						fp = fp->val.fptr;
					} else {
						break;
					}
					j++;
				}
				fpo = (FFJSONPrettyPrintPObj*) fpo->pObj;
				if (fpo != NULL) {
					pChildRootKey = fpo->name;
				}
			}
		}
		it++;
	};
	return NULL;
}

int FFJSON::getIndent (const char* ffjson, int* ci, int indent) {
	int i = *ci;
	if (ffjson[i] == '\n') {
		i++;
	} else if (ffjson[i] == '\r' && ffjson[i + 1] == '\n') {
		i += 2;
	} else {
		return indent;
	}
	int j = i + indent + 1;
	while (i < j) {
		if (ffjson[i] != '\t') {
			return indent;
		}
		i++;
	}
	return (indent + 1);
}

FFJSON::~FFJSON () {
	freeObj();
}

void FFJSON::freeObj (bool bAssignment) {
	switch (getType()) {
		case OBJ_TYPE::OBJECT: {
			map<string, FFJSON*>::iterator i;
			i = val.pairs->begin();
			while (i != val.pairs->end()) {
				delete i->second;
				++i;
			}
			delete val.pairs;
			break;
		}
		case OBJ_TYPE::ARRAY: {
			int i = val.array->size() - 1;
			while (i >= 0) {
				delete (*val.array)[i];
				--i;
			}
			delete val.array;
			break;
		}
		case OBJ_TYPE::SET_TYPE: {
			for (FFJSON* fp : *val.setPtr) {
				if (fp)
					delete fp;
			}
			delete val.setPtr;
			break;
		}
		case OBJ_TYPE::STRING: {}
		case OBJ_TYPE::XML: {
			delete[] val.string;
			break;
		}
		case LINK: {
			break;
		}
		case DLINK: {
			break;
		}
		case BINARY: {
			free(val.vptr);
			break;
		}
		case TIME: {
			delete val.m_pFerryTimeStamp;
		}
		default: {
			break;
		}
	}
	if (m_uFM.m_pFMH != nullptr) {
		destroyAllFeaturedMembers(bAssignment);
		m_uFM.m_pFMH=nullptr;
	}
	val.fptr =	0;
	size =		0;
	flags &=		bAssignment?0xF000FF00:0;
}
bool isWhite (char c) {
	switch (c) {
		case ' ':
		case '\n':
		case '\t':
		case '\r':
			return true;
		default:
			return false;
	}
	return false;
}
void FFJSON::trimWhites (string& s) {
	int i = 0;
	int j = s.length() - 1;
	while (i<=j && isWhite(s[i])) {
		++i;
	}
	while (i<=j && isWhite(s[j])) {
		--j;
	}
	s = i <= j ? s.substr(i, ++j - i) : "";
}

void FFJSON::trimQuotes (string& s) {
	int i = 0;
	int j = s.length() - 1;
	if (j<=0) {
		return;
	}
	if (s[0] == '"') {
		++i;
	}
	if (s[j] == '"') {
		--j;
	}
	++j;

	s = s.substr(i, j - i);
}

FFJSON::OBJ_TYPE FFJSON::objectType (string ffjson) {
	if (ffjson[0] == '{' && ffjson[ffjson.length() - 1] == '}') {
		return OBJ_TYPE::OBJECT;
	} else if (ffjson[0] == '"' && ffjson[ffjson.length() - 1] == '"') {
		return OBJ_TYPE::STRING;
	} else if (ffjson[0] == '[' && ffjson[ffjson.length() - 1] == ']') {
		return OBJ_TYPE::ARRAY;
	} else if (ffjson.compare("true") == 0 || ffjson.compare("false") == 0) {
		return OBJ_TYPE::BOOL;
	} else {
		return OBJ_TYPE::UNDEFINED;
	}
}

FFJSON& FFJSON::operator [] (void) {
	if (isLink()) {
		return (*val.fptr)[];
	} else if (isType(UNDEFINED)) {
	  settype:
		setType(SET_TYPE);
		val.setPtr= new ffset();
	} else if (isType(OBJECT) && size==0) {
		freeObj();
		goto settype;
	}
	FFJSON* obj= new FFJSON();
	if (isType(SET_TYPE)) {
		obj->setType(NEW_SET_MEMBER);
		obj->val.fptr= this;
	}
	return *obj;
}

FFJSON& FFJSON::operator [] (const string& prop) {
	return (*this)[prop.c_str()];
}

FFJSON& FFJSON::operator [] (const char* prop) {
	if (isLink()) {
		return (*val.fptr)[prop];
	} else if (isType(UNDEFINED)) {
		setType(OBJECT);
		FeaturedMember fmMapSequence;
		fmMapSequence.m_pvpsMapSequence =
			new vector<map<string, FFJSON*>::iterator>();
		insertFeaturedMember(fmMapSequence, FM_MAP_SEQUENCE);
		val.pairs = new map<string, FFJSON*>();
		size = 0;
	}
	FFJSON* obj = nullptr;
	if (isType(OBJ_TYPE::OBJECT)) {
		string t(prop);
		ffmap::iterator it = val.pairs->find(t);
		if (it != val.pairs->end()) {
			if (it->second != NULL) {
				FFJSON* prf = it->second;
				while (prf->isLink()) {
					prf = prf->val.fptr;
				}
				return *prf;
			} else {
				obj = new FFJSON();
				return *((*val.pairs)[t] = obj);
			}
		} else {
			obj = new FFJSON();
			FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
			++size;//should b increase only after getFeatruredMember
			pair<ffmap::iterator, bool> prNew = val.
				pairs->insert(pair<string, FFJSON*>(string(prop), obj));
			if (size < MAX_ORDERED_MEMBERS) {
				fmMapSequence.m_pvpsMapSequence->push_back(prNew.first);
			} else if (fmMapSequence.m_pvpsMapSequence) {
				nullFeaturedMember(FM_MAP_SEQUENCE);
			}
			return *obj;
		}
	} else if (isType(ARRAY)) {
		if (isEFlagSet(EXT_VIA_PARENT)) {
			FeaturedMember cFM = getFeaturedMember(FM_TABHEAD);
			return ((*this)[(*cFM.tabHead)[prop]]);
		} else {
			return ((*this)[atoi(prop)]);
		}
	} else if (isType(SET_TYPE)) {
		ffset::iterator it = val.setPtr->begin();
		while (it!=val.setPtr->end()) {
			FFJSON* pt = *it;
			++it;
			if (pt->isType(FFJSON::STRING)) {
				if (!strcmp(prop, pt->val.string)) {
					return *pt;
				}
			}
		}
	}
	if (!obj) {
		obj=&nullFFJSON;
	}
	return *obj;
}

FFJSON& FFJSON::operator [] (const int index) {
  ssstart:
	if (isType(OBJ_TYPE::ARRAY)) {
		if ((*val.array).size() > index) {
			if ((*val.array)[index] == NULL) {
				(*val.array)[index] = new FFJSON(NUL);
			} else if ((*val.array)[index]->isLink()) {
				FFJSON* prf = (*val.array)[index];
				while (prf->isLink()) {
					prf = prf->val.fptr;
				}
				return *prf;
			}
			return *((*val.array)[index]);
		} else {
			FFJSON* f;
			for (int i=size; i<=index;++i) {
				f = new FFJSON();
				val.array->push_back(f);
				++size;
			}
			return *f;
		}
	} else if (isType(UNDEFINED)) {
		setType(OBJ_TYPE::ARRAY);
		size = 0;
		val.array = new vector<FFJSON*>();
		FFJSON* f;
		for (int i=size; i<=index;++i) {
			f = new FFJSON();
			val.array->push_back(f);
			++size;
		}
		return *f;
	} else if (isLink()) {
		return (*val.fptr)[index];
	} else {
		return nullFFJSON;
	}
};

FFJSON& FFJSON::operator * () {
	FFJSON* fp= this;
	while (fp->isLink()) {
		fp= fp->val.fptr;
	}
	return *fp;
}

FFJSON* FFJSON::operator -> () {
	return &**this;
}

/**
 * converts FFJSON object to json string
 * @param encode_to_base64 if true then the binary data is base64 encoded
 * @return json string of this FFJSON object
 */
string FFJSON::stringify (bool json, bool bGetQueryStr,
								  FFJSONPObj * pObj, uint lnLvl) const {
	string ffs;
	stringify (ffs, json, bGetQueryStr, pObj, lnLvl);
	return ffs;
}
void FFJSON::stringify (
	string& ffs, bool json, bool bGetQueryStr, FFJSONPObj* pObj, uint lnLvl
) const {
	if (bGetQueryStr) {
		if (isQType(QUERY)) {
			ffs+= "?";
			return;
		} else if (isQType(DEL)) {
			ffs+= "delete";
			return;
		} else if (isQType(QUERY_TYPE::SET) || isType(OBJECT) ||
					  isType(ARRAY)) {
		} else {
			return;
		}
	}
	switch (getType()) {
	case OBJ_TYPE::STRING: {
		ffs.reserve(2 * size + 2);
		ffs+= '"';
		int i = 0;
		while (i < size) {
			switch (val.string[i]) {
			case '"':
				ffs+= "\\\"";
				break;
			case '\n':
				ffs+= "\\n";
				break;
			case '\r':
				ffs+= "\\r";
				break;
			case '\t':
				ffs+= "\\t";
				break;
			case '\\':
				ffs+= "\\\\";
				break;
			default:
				ffs+= val.string[i];
				break;
			}
			++i;
		}
		ffs+= '"';
		return;
	}
	case OBJ_TYPE::NUMBER: {
		if (isEFlagSet(PRECISION)) {
			int precision = getFeaturedMember(FM_PRECISION).precision;
			string num(toPreciseStr(val.number, precision));
			ffs+= num;
			return;
		} else {
			string num(toPreciseStr(val.number, 0));
			ffs+= num;
			return;
		}
	}
	case OBJ_TYPE::XML: {
		if (isEFlagSet(B64ENCODE)) {
			int output_length = 0;
			char * b64_char = base64_encode((const unsigned char*) val.string,
													  size, (size_t*) & output_length);
			ffs+= "\"";
			ffs.append(b64_char, output_length);
			ffs+= "\"";
			free(b64_char);
			return;
		} else {
			ffs+= "<xml length=\"";
			ffs+= to_string(size);
			ffs+= "\">";
			ffs.append(val.string, size);
			ffs+= "</xml>";
			return;
		}
	}
	case OBJ_TYPE::BOOL: {
		if (val.boolean) {
			ffs+= "true";
			return;
		} else {
			ffs+= "false";
			return;
		}
	}
	case OBJ_TYPE::OBJECT: {
		if (size==0) {
			ffs+= "{}";
			return;
		}
		map<string, FFJSON*>& objmap= *(val.pairs);
		ffs+= "{";
		map<string, FFJSON*>::iterator i;
		FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
		int iMapSeqIndexer= 0;
		if (fmMapSequence.m_pvpsMapSequence) {
			i= (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
		} else {
			i= objmap.begin();
		}
		FFJSONPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<FFJSON*> (this);
		while (i!= objmap.end()) {
			uint32_t t= i->second ? i->second->getType() : NUL;
			if (t!= UNDEFINED && !i->second->isEFlagSet(COMMENT)) {
				if (t!= NUL) {
					if (isEFlagSet(B64ENCODE))i->second->setEFlag(B64ENCODE);
					if ((isEFlagSet(B64ENCODE_CHILDREN))&&
						 !isEFlagSet(B64ENCODE_STOP))
						i->second->setEFlag(B64ENCODE_CHILDREN);
				}
				if (json){ffs+= '"';};
				ffs+= i->first;
				if (json){ffs+= '"';};
				ffs+= ':';
				lfpo.name= &i->first;
				if (t!= NUL) {
					i->second->stringify(ffs, json, false, &lfpo, lnLvl);
				} else if (json) {
					ffs.append("null");
				}
				ffs.append(",");
			}
			if (fmMapSequence.m_pvpsMapSequence) {
				if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->size()) {
					i= (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
				} else {
					i= objmap.end();
				}
			} else {
				++i;
			}
		}
		ffs.back()= '}';
		break;
	}
	case OBJ_TYPE::SET_TYPE: {
		ffset& objset = *(val.setPtr);
		FFJSONPrettyPrintPObj lfpo(NULL, NULL, NULL, NULL);
		lfpo.pObj= pObj;
		ffs+= "{";
		for (FFJSON* fp : objset) {
			fp->stringify(ffs, json, false, &lfpo, lnLvl);
			ffs+= ',';
		}
		ffs.back()= '}';
		break;
	}
	case OBJ_TYPE::ARRAY: {
		vector<FFJSON*>& objarr= *(val.array);
		ffs+= "[";
		int i= 0;
		FFJSONPrettyPrintPObj lfpo(NULL, NULL, NULL, NULL);
		lfpo.pObj= pObj;
		lfpo.value= const_cast<FFJSON*> (this);
		while (i < objarr.size()) {
			uint32_t t= objarr[i]? objarr[i]->getType(): NUL;
			if (t== NUL || t== UNDEFINED) {
				if (json) {
					ffs.append("null");
				}
			} else {
				if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
				if ((isEFlagSet(B64ENCODE_CHILDREN))&&
					 !isEFlagSet(B64ENCODE_STOP))
					objarr[i]->setEFlag(B64ENCODE_CHILDREN);
				string str= objarr[i]->stringify(json, false, &lfpo, lnLvl);
				ffs.append(str);
			}
			if (++i != objarr.size()) {
				if (objarr[i]) {
					ffs.append(",");
				}
			}
		}
		ffs+= ']';
		break;
	}
	case LINK: {
		vector<string>* vtProp = getFeaturedMember(FM_LINK).link;
		if ((lnLvl<=0 || returnNameIfDeclared(*vtProp, pObj) != NULL) &&
//if ((returnNameIfDeclared(*vtProp, pObj) != NULL) &&
			 !isEFlagSet(LONG_LAST_LN)
		) {
			string ln = implode(".", *vtProp);
			ffs+= json?"\""+ln+"\"":ln;
			return;
		} else {
			val.fptr->stringify(ffs, json, false, pObj,
									  lnLvl?lnLvl-1:0);
			return;
		}
	}
	case DLINK: {
		val.fptr->stringify(ffs, json, false, pObj, lnLvl?lnLvl-1:0);
		return;
	}
	case BINARY: {
		string str= to_string(size);
		ffs += "(" + str + ")";
		ffs.append((const char*) val.vptr, size);
		break;
	}
	case TIME: {
		string str= (string) (*val.m_pFerryTimeStamp);
		ffs+= str;
		break;
	}
	case VPTR: {
		ffs+= "0x";
		ffs+= to_string((size_t)val.vptr);
		break;
	}
	default: {
		if (!isQType(NONE)) {
			if (isQType(QUERY)) {
				ffs+= "?";
				return;
			} else if (isQType(DEL)) {
				ffs+= "delete";
				return;
			} else {
				return;
			}
		} else {
			ffs+= "null";
		}
	}}
	if (isEFlagSet(EXTENDED) && !isType(STRING)) {
		FFJSON* pParent= getFeaturedMember(FM_PARENT).m_pParent;
		ffs+= '|';
		pParent->stringify(ffs, false, false, pObj,lnLvl);
	}
	return;
}

string FFJSON::prettyString (
	bool json, bool printComments, int indent,
	FFJSONPrettyPrintPObj* pObj, bool printFilePath, bool save
) const {
	string ps;
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE)) && pObj && printFilePath) {
		ccp filename = getFeaturedMember(FM_FILE).m_sFileName;
		if (save)
			this->save(json, printComments, 0, pObj, false);
		int ri = strlen(filename)-1;
		while (ri>0 && (filename[ri]!='.' || filename[ri+1]!='/')) {
			--ri;
		}
		if (ri>0)
			ri+=2;
		return string("file://")+(filename+ri)+
			(isEFlagSet(CASTFILE)?string("|")+OBJ_STR[getType()]:"");
	}
	if (!printFilePath && save) {
		printFilePath = true;
	}
	if (save && isEFlagSet(CASTFILE)) {
		indent=-1;
	}
	switch (getType()) {
		case OBJ_TYPE::STRING: {
			ps = "\"";
			char* pcNewLnCharPos = strchr(val.string, '\n');
			bool hasNewLine = (pcNewLnCharPos != NULL);
			if (pObj && pObj->m_bGiveFirstLine) {
				if (hasNewLine) {
					ps.append(val.string, 0, pcNewLnCharPos - val.string);
					pObj->m_bGiveFirstLine = false;
				} else {
					ps.append(val.string);
					ps += '"';
				}
			} else {
				if (hasNewLine) {
					ps += '\n';
					ps.append(indent + 1, '\t');
				}
				int stringNail = 0;
				int i = 0;
				if (hasNewLine) {
					for (i = 0; i < size; i++) {
						if (val.string[i] == '\n') {
							ps.append(val.string, stringNail, i + 1 - stringNail);
							ps.append(indent + 1, '\t');
							stringNail = i + 1;
						}
					}
				} else {
					i = size;
				}
				int ii = stringNail;
				while (ii < size) {
					char ch = val.string[ii];
					switch (ch) {
						case '"':
							if (hasNewLine) {
								break;
							}
						case '\\':
							ps+='\\';
							break;
					}
					ps+=ch;
					++ii;
				}
				if (hasNewLine) {
					ps += '\n';
					ps.append(indent>0?indent:0, '\t');
				}
				ps += "\"";
			}
			break;
		}
		case OBJ_TYPE::NUMBER: {
			if (isEFlagSet(PRECISION)) {
				int precision = getFeaturedMember(FM_PRECISION).precision;
				string num(toPreciseStr(val.number, precision));
				return num;
			} else {
				return toPreciseStr(val.number, 0);
			}
			break;
		}
		case OBJ_TYPE::XML: {
			if (isEFlagSet(B64ENCODE)) {
				int output_length = 0;
				char * b64_char = base64_encode(
					(const unsigned char*) val.string,
					size, (size_t*) & output_length);
				string b64_str(b64_char, output_length);
				free(b64_char);
				return ("\"" + b64_str + "\"");
			} else {
				return ("<xml length = \"" + to_string(size) + "\" >" +
						  string(val.string, size) + "</xml>");
			}
			break;
		}
		case OBJ_TYPE::BOOL: {
			if (val.boolean) {
				return ("true");
			} else {
				return ("false");
			}
			break;
		}
		case OBJ_TYPE::OBJECT: {
			if (size==0) {
				return (save && isEFlagSet(CASTFILE))?"":"{}";
			}
			map<string, FFJSON*>& objmap = *(val.pairs);
			map<string, FFJSON*>::iterator i;
			map<string, vector<int> > msviClWidths;
			FeaturedMember fmMapSequence = getFeaturedMember(FM_MAP_SEQUENCE);
			int iMapSeqIndexer = 0;
			if (fmMapSequence.m_pvpsMapSequence) {
				if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->size()) {
					i = (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
				} else {
					i = objmap.end();
				}
			} else {
				i = objmap.begin();
			}
			bool notComment = false;
			bool hasComment = false;
			map<string*, const string*> memKeyFFPairMap;
			list<string> ffPairLst;
			map<const string*, const string*> deps;
			map<const string*, list<string>::iterator> mpKeyPrettyStringItMap;
			FFJSONPrettyPrintPObj lfpo(&deps, &ffPairLst, &memKeyFFPairMap,
												&mpKeyPrettyStringItMap);
			lfpo.pObj = pObj;
			lfpo.value = const_cast<FFJSON*> (this);
			lfpo.m_lsFFPairLst = &ffPairLst;
			lfpo.m_mpMemKeyFFPairMap = &memKeyFFPairMap;
			lfpo.m_mpDeps = &deps;
			lfpo.m_msviClWidths = &msviClWidths;
			int iLastNwLnIndex = 0;
			if (isEFlagSet(EXT_VIA_PARENT)) {
				if (isEFlagSet(EXTENDED)) {
					iLastNwLnIndex = 1;

				} else {
					ffl_err(FFJ_MAIN, "An object never extends via parent!");
					return "";
				}
			} else if (isEFlagSet(HAS_CHILDREN)) {

			} else {
			}
			ffPairLst.push_back(string());
			list<string>::iterator itPretty = ffPairLst.begin();
			while (i != objmap.end()) {
				string& ms = *itPretty;
				uint32_t t = (i->second || i->second->isType(LINK)) ?
					i->second->getType() : NUL;
				notComment = !i->second->isEFlagSet(COMMENT);
				hasComment = i->second->isEFlagSet(HAS_COMMENT);
				if (t != UNDEFINED && t != NUL && notComment) {
					if (isEFlagSet(B64ENCODE))i->second->setEFlag(B64ENCODE);
					if ((isEFlagSet(B64ENCODE_CHILDREN))&&!isEFlagSet(
							 B64ENCODE_STOP))
						i->second->setEFlag(B64ENCODE_CHILDREN);
					if (hasComment && !json && printComments) {
						string name("#");
						name += i->first;
						map<string, FFJSON*>::iterator ci = val.pairs->find(name);
						if (ci != val.pairs->end()) {
							ms += "\n";
							ms.append(indent + 1, '\t');
							//memKeyFFPairMap[name] = &ms;
							ms += name + ": ";
							lfpo.name = &name;
							ms += ci->second->prettyString(
								json, printComments,indent + 1, &lfpo,
								printFilePath, save);
							ms += ",\n";
						}
					}
					ms.append(indent + 1, '\t');
					if (json) ms += "\"";
					ms += i->first;
					memKeyFFPairMap[&ms] = &(i->first);
					mpKeyPrettyStringItMap[&i->first] = itPretty;
					lfpo.name = &i->first;
					if (json) ms += "\"";
					ms += ": ";
					ms.append(i->second->prettyString(
									 json, printComments, indent +1,
									 &lfpo, printFilePath, save));
				} else if (t == NUL) {
					ms.append(indent + 1, '\t');
					if (json)ms.append("\"");
					ms += i->first;
					memKeyFFPairMap[&ms] = &i->first;
					if (json)ms += "\"";
					ms += ": ";
				}
				if (t != UNDEFINED && notComment) {
					//comment already gets its ",\n" above.
					ms.append(",\n");
					if (hasComment && notComment && !json && printComments) {
						ms += '\n';
					}
					ffPairLst.push_back(string());
					++itPretty;
				}
				if (fmMapSequence.m_pvpsMapSequence) {
					if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->size()) {
						i = (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
					} else {
						i = objmap.end();
					}
				} else {
					++i;
				}
			}
			ffPairLst.pop_back();
			//headTheHeader(lfpo);
			ps=(save && isEFlagSet(CASTFILE))?"":"{";
			if (ffPairLst.size() > 0) {
				string& rLastKeyValStr = ffPairLst.back();
				const string& key = *memKeyFFPairMap[&rLastKeyValStr];
				FFJSON* fj = (*val.pairs)[key];
				if (printComments && fj->isEFlagSet(HAS_COMMENT)) {
					rLastKeyValStr.erase(rLastKeyValStr.length() - 3);
				} else {
					rLastKeyValStr.erase(rLastKeyValStr.length() - 2);
				}
				rLastKeyValStr += '\n';
				itPretty = ffPairLst.begin();
				if (!(save && isEFlagSet(CASTFILE)))
					ps += "\n";
				while (itPretty != ffPairLst.end()) {
					ps += *itPretty;
					++itPretty;
				}
				ps.append(indent>0?indent:0, '\t');
			}
			if (!(save && isEFlagSet(CASTFILE)))
				ps.append("}");
			break;
		}
		case OBJ_TYPE::SET_TYPE: {
			ffset& objset = *(val.setPtr);
			FFJSONPrettyPrintPObj lfpo(NULL, NULL, NULL, NULL);
			lfpo.pObj = pObj;
			lfpo.value = const_cast<FFJSON*> (this);
			ps = (save && isEFlagSet(CASTFILE))?"":"{";
			if(objset.size()) {
				for (FFJSON* fp : objset) {
					ps += fp->prettyString(json, printComments, indent+1, &lfpo,
												  printFilePath, save);
					ps+=',';
				}
				ps.pop_back();
			}
			if (!(save && isEFlagSet(CASTFILE)))
				ps+='}';
			break;
		}
		case OBJ_TYPE::ARRAY: {
			vector<FFJSON*>& objarr = *(val.array);
			int iLastNwLnIndex = 0;
			vector<int> vClWidths;
			map<string, vector<int> > msviClWidths;
			FFJSONPrettyPrintPObj lfpo(NULL, NULL, NULL, NULL);
			lfpo.pObj = pObj;
			lfpo.value = const_cast<FFJSON*> (this);
			lfpo.m_msviClWidths = &msviClWidths;
			int iParentHeight = 0;
			if (isEFlagSet(EXT_VIA_PARENT)) {
				if (isEFlagSet(EXTENDED)) {
					ps = (save && isEFlagSet(CASTFILE))?"":"[\n";
					iLastNwLnIndex = 1;
				} else {
					iParentHeight = 1;
					FFJSONPrettyPrintPObj* pFFPPPObjHolder =
						static_cast<FFJSONPrettyPrintPObj*> (pObj->pObj);
					string sChildName = *pFFPPPObjHolder->name;
					while (pFFPPPObjHolder && pFFPPPObjHolder->m_msviClWidths->
							 find(sChildName)
							 == pFFPPPObjHolder->m_msviClWidths->end()) {
						pFFPPPObjHolder = static_cast<FFJSONPrettyPrintPObj*>
							(pFFPPPObjHolder->pObj);
						if (pFFPPPObjHolder)
							sChildName = *pFFPPPObjHolder->name + '.' + sChildName;
						iParentHeight++;
					}
					if (!pFFPPPObjHolder) {
						ffl_err(FFJ_MAIN, "ColumnWidths not found");
						return "";
					}
					vClWidths = (*pFFPPPObjHolder->
									 m_msviClWidths)[sChildName];
					ps = (save && isEFlagSet(CASTFILE))?"":"[";
					int iNameLength = 0;
					if (pObj && pObj->value->isType(OBJECT)) {
						iNameLength = pObj->name->length() + 3;
					}
					ps.append((((vClWidths[0] + 7) / 8)*8 - 8 * iParentHeight -
								  iNameLength + 7) / 8, '\t');
					lfpo.m_bGiveFirstLine = true;
				}
			} else if (isEFlagSet(HAS_CHILDREN)) {
				ps = (save && isEFlagSet(CASTFILE))?"":"[";
				vector<FFJSON*>* pvpfjChildren = getFeaturedMember(FM_CHILDREN).
					m_pvChildren;
				if (pvpfjChildren->size() > 0) {
					vClWidths.resize(size + 1, 0);
					vClWidths[0] = pObj->name->length() + 3;
					for (int i = 0; i < pvpfjChildren->size(); i++) {
						iLastNwLnIndex = 1;
						map<string, int>& mTabHead = *(*pvpfjChildren)[i]->val.fptr
							->getFeaturedMember(FM_TABHEAD).tabHead;
						map<string, int>::iterator itTabHead = mTabHead.begin();
						FFJSON* pfjChild = (*pvpfjChildren)[i]->val.fptr;
						map<string, FFJSON*>::iterator itmspfTrueChild;
						vector<string>* vsLink = (*pvpfjChildren)[i]->
							getFeaturedMember(FM_LINK).link;
						while (itTabHead != mTabHead.end()) {
							int iCurColWidth = itTabHead->first.size() + 3;
							int iCurColIndex = itTabHead->second;
							vClWidths[iCurColIndex + 1] = iCurColWidth;
							if (pfjChild->isType(OBJECT))
								itmspfTrueChild = pfjChild->val.pairs->begin();
							for (int j = 0; j < pfjChild->size; ++j) {
								FFJSON* pfjChildMem;
								if (pfjChild->isType(ARRAY)) {
									pfjChildMem = (*(*pfjChild->val.array)[j]->val.
														array)
										[iCurColIndex];
								} else if (pfjChild->isType(OBJECT)) {
									pfjChildMem = (*itmspfTrueChild->second->val.
														array)
										[iCurColIndex];
									++itmspfTrueChild;
								}
								unsigned int width = pfjChildMem->getFeaturedMember
									(FM_WIDTH).width;
								if (pfjChildMem->isType(STRING)) {
									if (pfjChildMem->isEFlagSet(LONG_LAST_LN))
										width += 3;
									else if (pfjChildMem->isEFlagSet(
													ONE_SHORT_LAST_LN))
										width += 2;
									else
										width += 1;
								} else {
									width += 1;
								}
								if (iCurColWidth < width) {
									iCurColWidth = width;
									vClWidths[iCurColIndex + 1] = width;
								}
							}
							itTabHead++;
						}
						if (pfjChild->isType(OBJECT)) {
							itmspfTrueChild = pfjChild->val.pairs->begin();
							while (itmspfTrueChild != pfjChild->val.pairs->end()) {
								int iCurChldInitIndent = (vsLink->size())*8 +
									itmspfTrueChild->first.length() + 3;
								if (iCurChldInitIndent > vClWidths[0])
									vClWidths[0] = iCurChldInitIndent;
								itmspfTrueChild++;
							}
						}
						// set the initial indent in 0th index
					}
					for (int i = 0; i < pvpfjChildren->size(); i++) {
						vector<string>* vsLink = (*pvpfjChildren)[i]->
							getFeaturedMember(FM_LINK).link;
						string sChild = implode(".", (*vsLink));
						(*pObj->m_msviClWidths)[sChild] = vClWidths;
					}
				}
				lfpo.m_bGiveFirstLine = true;
				ps.append((((vClWidths[0] + 7) / 8)*8 - pObj->name->length() + 7) /
							 8, '\t');
			} else {
				ps = (save && isEFlagSet(CASTFILE))?"":"[\n";
			}
			int i = 0;
			bool bInCompleteStrs = false;
			vector<FFJSON*> vpfjMulLnStrs;
			while (i < objarr.size()) {
				uint32_t t = objarr[i] ? objarr[i]->getType() : NUL;
				string sMem;
				if (t != UNDEFINED && t != NUL) {
					if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
					if ((isEFlagSet(B64ENCODE_CHILDREN))&&
						 !isEFlagSet(B64ENCODE_STOP))
						objarr[i]->setEFlag(B64ENCODE_CHILDREN);
					if (!(isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) &&
						 !isEFlagSet(HAS_CHILDREN))
						ps.append(indent + 1, '\t');
					string name = to_string(i);
					lfpo.name = &name;
					if (objarr[i]->isType(STRING) &&
						 (isEFlagSet(HAS_CHILDREN) ||
						  isEFlagSet(EXT_VIA_PARENT))) {

					}
					sMem = objarr[i]->prettyString(
						json, printComments, indent + 1,&lfpo,printFilePath,save);
					ps.append(sMem);
				} else if (t == NUL) {
					ps.append(indent + 1, '\t');
				}
				int width = sMem.length();
				if ((isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) ||
					 isEFlagSet(HAS_CHILDREN)
				) {
					bInCompleteStrs |= !lfpo.m_bGiveFirstLine;
					if (lfpo.m_bGiveFirstLine) {
						vpfjMulLnStrs.push_back(NULL);
						if (i + 1 != objarr.size()) {
							ps += ',';
							width++;
						}
					} else {
						lfpo.m_bGiveFirstLine = true;
						vpfjMulLnStrs.push_back(objarr[i]);
					}
					ps.append((((vClWidths[i + 1] + (((i + 1) < objarr.size()) ? 8 :
																6)) / 8)*8 - width + 7) / 8, '\t');
				} else {
					if (i + 1 != objarr.size()) {
						ps.append(",\n");
					} else {
						ps.append("\n");
					}
				}
				if (++i != objarr.size()) {

				} else {
					// if ((isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) ||
					//		  isEFlagSet(HAS_CHILDREN)
					// ) {
					//		ps.append("\n");
					// }
				}
			}
			if (bInCompleteStrs) {
				ps.append(
					ConstructMultiLineStringArray(
						vpfjMulLnStrs, indent,
						vClWidths
					)
				);
			}
			if ((isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) ||
				 isEFlagSet(HAS_CHILDREN)
			) {
			} else {
				ps.append(indent, '\t');
			}
			if (!(save && isEFlagSet(CASTFILE)))
				ps.append("]");
			break;
		}
		case LINK: {
			vector<string>* vtProp = getFeaturedMember(FM_LINK).link;
			if (save || returnNameIfDeclared(*vtProp, pObj) != NULL) {
				string ln = implode(".", *vtProp);
				return json?"\""+ln+"\"":ln;
			} else {
				return val.fptr->prettyString(
					json, printComments, indent, pObj, printFilePath, save);
			}
			break;
		}
		case DLINK: {
				return val.fptr->prettyString(
					json, printComments, indent + 1,
					pObj, printFilePath, save);
		}
		case BINARY: {
			ps += "(" + to_string(size) + ")";
			ps.append((const char*) val.vptr, size);
			break;
		}
		case TIME:
			return (string) (*val.m_pFerryTimeStamp);
		default:
			if (!isQType(NONE)) {
				if (isQType(QUERY)) {
					return "?";
				} else if (isQType(DEL)) {
					return "delete";
				}
			} else {
				ps += "null";
			}
	}
	if (isEFlagSet(EXTENDED) && !isType(STRING)) {
		FFJSON* pParent = getFeaturedMember(FM_PARENT).m_pParent;
		ps += "|";
		ps += pParent->stringify(false, false, pObj);
	}
	return ps;
}

string FFJSON::ConstructMultiLineStringArray (
	vector<FFJSON*>& vpfMulLnStrs, int indent, vector<int>& vClWidths
) const {
	string sProduct;
	int iLineIndex = 1;
	bool bAreMulLnStrsRemained = false;
	do {
		bAreMulLnStrsRemained = false;
		sProduct += '\n';
		sProduct.append(indent + ((vClWidths[0] - 8 + 7) / 8), '\t');
		for (int i = 0; i < vpfMulLnStrs.size(); i++) {
			if (vpfMulLnStrs[i]) {
				int iCurLnInd = 0;
				char* pcNwLn = vpfMulLnStrs[i]->val.string;
				while (iCurLnInd < iLineIndex && pcNwLn) {
					pcNwLn = strchr(pcNwLn, '\n');
					if (pcNwLn)pcNwLn++;
					iCurLnInd++;
				}
				char* pcNxtLn = NULL;
				if (pcNwLn)pcNxtLn = strchr(pcNwLn, '\n');
				int width = 0;
				if (pcNxtLn) {
					sProduct += ' ';
					width = pcNxtLn - pcNwLn;
					sProduct.append(pcNwLn, width);
					sProduct.append((((vClWidths[i + 1] + 8) / 8)*8 - (int)
										  width - 1 + 7) / 8, '\t');
					bAreMulLnStrsRemained |= true;
				} else if (pcNwLn) {
					pcNxtLn = pcNwLn;
					while (*pcNxtLn)pcNxtLn++;
					sProduct += ' ';
					sProduct.append(pcNwLn);
					int cwidth =
						((vClWidths[i+1]+(((i+1)<vpfMulLnStrs.size())?8:6))/8)*8-
						(int)(pcNxtLn-pcNwLn)-3+7;
					if (i < vpfMulLnStrs.size() - 1) {
						sProduct += "\",";
						sProduct.append(cwidth/8, '\t');
					} else {
						sProduct += '"';
						sProduct.append((cwidth+1)/8, '\t');
					}
					vpfMulLnStrs[i] = NULL;
				}
			} else {
				sProduct.append((vClWidths[i + 1] + (((i + 1) < vpfMulLnStrs.
																  size()) ? 8 : 6)) / 8, '\t');
			}
		}
		iLineIndex++;
	} while (bAreMulLnStrsRemained);
	return sProduct;
}

FFJSON::operator const char* () {
	return isLink() ? val.fptr->val.string : val.string;
}

FFJSON::operator double () {
	return isLink() ? val.fptr->val.number : val.number;
}

FFJSON::operator float () {
	return (float) isLink() ?
		val.fptr->val.number : (isType(STRING)? atof(val.string): val.number);
}

FFJSON::operator bool () {
	FFJSON* fp = this;
	if (isLink()) {
		fp = val.fptr;
	};
	OBJ_TYPE t = fp->getType();
	switch(t) {
		case BOOL:
			return fp->val.boolean;
		case UNDEFINED:
		case NUL:
			return false;
		case STRING:
			return size;
		case NUMBER:
			return (bool)fp->val.number;
		default:
			return true;
	}
}

FFJSON::operator int () {
	if (isLink()) {
		return (int) (val.fptr->val.number);
	}
	return (int) val.number;
}

FFJSON::operator long () {
	if (isLink()) {
		return (long) (val.fptr->val.number);
	}
	return (long) val.number;
}

FFJSON::operator unsigned int () {
	if (isLink()) {
		return (unsigned int) (val.fptr->val.number);
	}
	return (unsigned int) val.number;
}

FFJSON& FFJSON::operator = (Blob_ b) {
	if (isQType(UPDATE)) {
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	FFJSON* parent = nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent = val.fptr;
	}
	freeObj(true);
	setType(BINARY);
	size = b.s;
	val.vptr = b.p;
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			return nullFFJSON;
		}
	}
	return *this;
}
// FFJSON& FFJSON::operator = (char* s) {
// 	return (*this)= (ccp)s;
// }
FFJSON& FFJSON::operator = (const char* s) {
	if (isQType(UPDATE)) {
		FeaturedMember fm= getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	FFJSON* parent = nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent = val.fptr;
	}
	freeObj(true);
	int i = 0;
	int j = strlen(s);
	if (s[0] == '<' && s[j-1]=='>') {
		i++;
		int xmlNail = i;
		string xmlTag;
		int length = -1;
		bool tagset = false;
		while (s[i] != '>' && i < j) {
			if (s[i] == ' ') {
				tagset = true;
				if (s[i + 1] == 'l' &&
					 s[i + 2] == 'e' &&
					 s[i + 3] == 'n' &&
					 s[i + 4] == 'g' &&
					 s[i + 5] == 't' &&
					 s[i + 6] == 'h') {
					i += 7;
					while (s[i] != '=' && i < j) {
						i++;
					}
					i++;
					while (s[i] != '"' && i < j) {
						i++;
					}
					i++;
					string lengthstr;
					while (s[i] != '"' && i < j) {
						lengthstr += s[i];
						i++;
					}
					length = atoi(lengthstr.c_str());
				}
			} else if (!tagset) {
				xmlTag += s[i];
			}
			i++;
		}
		setType(XML);
		i++;
		xmlNail = i;
		if (length>-1 && length < (j - i)) {
			i += length;
		}
		while (i < j) {
			if (s[i] == '<' &&
				 s[i + 1] == '/') {
				if (xmlTag.compare(0, xmlTag.length(), s + i + 2, xmlTag.length())
					 == 0 && s[i + 2 + xmlTag.length()] == '>') {
					size = i - xmlNail;
					val.string = new char[size + 1];
					memcpy(val.string, s + xmlNail,
							 size);
					val.string[size] = '\0';
					i += 3 + xmlTag.length();
					break;
				}
			}
			i++;
		}
	} else {
		setType(STRING);
		size = strlen(s);
		val.string = new char[size + 1];
		int iLastNewLnIndex = 0;
		FeaturedMember fmWidth;
		fmWidth.width = 0;
		int i = 0;
		for (i = 0; i < size; i++) {
			if (s[i] == '\n') {
				if (i - iLastNewLnIndex > fmWidth.width)
					fmWidth.width = i - iLastNewLnIndex;
				iLastNewLnIndex = i;
			}
			val.string[i] = s[i];
		}
		if (i - iLastNewLnIndex >= fmWidth.width) {
			fmWidth.width = i - iLastNewLnIndex;
			setEFlag(LONG_LAST_LN);
		} else if (i - iLastNewLnIndex == fmWidth.width - 1) {
			setEFlag(ONE_SHORT_LAST_LN);
		}
		insertFeaturedMember(fmWidth, FM_WIDTH);
		val.string[size] = '\0';
	}
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			return nullFFJSON;
		}
	}
	return *this;
}

FFJSON& FFJSON::operator = (const string& s) {
	operator=(s.c_str());
	return *this;
}

FFJSON& FFJSON::operator = (const int& i) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	FFJSON* parent = nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent = val.fptr;
	}
	freeObj(true);
	setType(NUMBER);
	val.number = i;
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			return nullFFJSON;
		}
	}
	return *this;
}

FFJSON& FFJSON::operator = (const FFJSON& f) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	FFJSON* parent = nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent = val.fptr;
	}
	if (f.isType(UNDEFINED)) {
		freeObj();
		setType(UNDEFINED);
		return *this;
	}
	if (!(isType(OBJECT) && f.isType(OBJECT)))
		freeObj(true);
	copy(f, COPY_ALL);
	return *this;
}

// need to implement, segfaults during stringify but
// can be used to hold pointers
FFJSON& FFJSON::operator = (FFJSON* f) {
	if (this==f)
		return *this;
	if (isQType(UPDATE)) {
		FeaturedMember fm= getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(DLINK);
	val.fptr= f;
	return *this;
}

FFJSON& FFJSON::addLink (const FFJSON& PObj, string label) {
	vector<string>* prop = new vector<string>();
	explode(".", label, *prop);
	FFJSON* obj = const_cast<FFJSON*>(PObj.returnNameIfDeclared(*prop));
	if (obj) {
		FFJSON* parent = nullptr;
		if (!isType(NEW_SET_MEMBER))
			freeObj(true);
		else {
			parent = val.fptr;
		}
		setType(LINK);
		val.fptr = obj;
		FeaturedMember cFM;
		cFM.link = prop;
		insertFeaturedMember(cFM, FM_LINK);
		if (parent) {
			if (parent->val.setPtr->insert(this).second)
				++parent->size;
			else {
				delete this;
				return nullFFJSON;
			}
		}
	} else {
		delete prop;
	}
	return *this;
}

FFJSON& FFJSON::addLink (const string&& objPath, const string&& linkPath) {
	vector<string>* objProp = new vector<string>();
	explode(".", objPath, *objProp);
	FFJSON* obj = const_cast<FFJSON*>(this->returnNameIfDeclared(*objProp));
	FFJSON* link = this;
	if (obj) {
		FFJSON* parent = NULL;
		vector<string>* linkProp = new vector<string>();
		explode(".", linkPath, *linkProp);
		for (int i=0; i<linkProp->size();++i) {
			if ((*linkProp)[i]=="*") {
				link=&(*link)[];
			} else {
				link=&(*link)[(*linkProp)[i]];
			}
			if (i<linkProp->size()-1) {
				objProp->insert(objProp->begin(), "");
			}
		}
		if (!link->isType(NEW_SET_MEMBER))
			link->freeObj(true);
		else {
			parent = link->val.fptr;
		}
		link->setType(LINK);
		link->val.fptr = obj;
		FeaturedMember cFM;
		cFM.link = objProp;
		link->insertFeaturedMember(cFM, FM_LINK);
		if (parent) {
			if (parent->val.setPtr->insert(link).second)
				++parent->size;
			else {
				delete link;
				link=&nullFFJSON;
			}
		}
		delete linkProp;
		return *link;
	} else {
		delete objProp;
	}
	return *link;
}

FFJSON& FFJSON::operator = (const double& d) {
	if (isQType(UPDATE)) {
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	FeaturedMember cFM;
	cFM.precision = 16;
	setEFlag(PRECISION);
	insertFeaturedMember(cFM, FM_PRECISION);
	setType(NUMBER);
	val.number = d;
	return *this;
}

FFJSON& FFJSON::operator = (const float& f) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	FeaturedMember cFM;
	cFM.precision = 8;
	setEFlag(PRECISION);
	insertFeaturedMember(cFM, FM_PRECISION);
	setType(NUMBER);
	val.number = f;
	return *this;
}

FFJSON& FFJSON::operator = (const long& l) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number = l;
	return *this;
}

FFJSON& FFJSON::operator = (const short& s) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number = s;
	return *this;
}

FFJSON& FFJSON::operator = (const unsigned int& i) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number = i;
	return *this;
}

FFJSON& FFJSON::operator = (const bool& b) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(BOOL);
	val.boolean = b;
	return *this;
}
void FFJSON::trim() {
	if (isType(OBJECT)) {
		int i;
		FeaturedMember fmMapSeq = getFeaturedMember(FM_MAP_SEQUENCE);
		vector<map<string, FFJSON*>::iterator >* vmpspfPairs =
			fmMapSeq.m_pvpsMapSequence;
		i = size - 1;
		while (i >= 0 && vmpspfPairs) {
			if (((*(*vmpspfPairs)[i]->second).isType(UNDEFINED)
				  &&!(*(*vmpspfPairs)[i]->second).isQType(NONE)) || (*(*vmpspfPairs)[i]->second).isType(NUL)) {
				delete (*vmpspfPairs)[i]->second;
				val.pairs->erase((*vmpspfPairs)[i]);
				vmpspfPairs->erase(vmpspfPairs->begin() + i);
				--size;
			}
			--i;
		}
	} else if (isType(ARRAY)) {
		if ((*this)[size - 1].isType(UNDEFINED)) {
			delete (*val.array)[size - 1];
			val.array->pop_back();
			--size;
		}
		int i = 0;
		while (i < val.array->size()) {
			if ((*val.array)[i] != NULL) {
				if (((*val.array)[i]->isType(UNDEFINED)&&
					  !(*val.array)[i]->isQType(NONE)) ||
					 (*val.array)[i]->isType(NUL)) {

					delete (*val.array)[i];
					(*val.array)[i] = NULL;
				}
			}
			i++;
		}
	}
}

string FFJSON::queryString() {
	if (isType(OBJ_TYPE::STRING)) {
		if (isQType(QUERY_TYPE::SET)) {
			return ("\"" + string(val.string, size) + "\"");
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::NUMBER)) {
		if (isQType(QUERY_TYPE::SET)) {
			return to_string(val.number);
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::XML)) {
		if (isQType(QUERY_TYPE::SET)) {
			if (isEFlagSet(B64ENCODE)) {
				int output_length = 0;
				char * b64_char = base64_encode((const unsigned char*)
														  val.string, size, (size_t*) & output_length);
				string b64_str(b64_char, output_length);
				free(b64_char);
				return ("\"" + b64_str + "\"");
			} else {
				return ("<xml length=\"" + to_string(size) + "\">" +
						  string(val.string, size) + "</xml>");
			}
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::BOOL)) {
		if (isQType(QUERY_TYPE::SET)) {
			if (val.boolean) {
				return ("true");
			} else {
				return ("false");
			}
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::OBJECT)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			string ffs;
			map<string, FFJSON*>& objmap = *(val.pairs);
			ffs = "{";
			map<string, FFJSON*>::iterator i;
			i = objmap.begin();
			bool matter = false;
			while (i != objmap.end()) {
				string ffjsonStr;
				uint32_t t = (i->second != NULL) ? i->second->getType() : NUL;
				if (t != UNDEFINED || (t != NUL && !i->second->isQType(NONE))) {
					if (t != NUL) {
						if (isEFlagSet(B64ENCODE))i->second->
															  setEFlag(B64ENCODE);
						if ((isEFlagSet(B64ENCODE_CHILDREN))&&
							 !isEFlagSet(B64ENCODE_STOP))
							i->second->setEFlag(B64ENCODE_CHILDREN);
						ffjsonStr = i->second->queryString();
					}
					if (ffjsonStr.length() > 0) {
						if (matter)ffs.append(",");
						ffs.append("\"" + i->first + "\":");
						ffs.append(ffjsonStr);
						matter = true;
					}
				}
				i++;
			}
			if (ffs.length() == 1) {
				ffs = "";
			} else {
				ffs += '}';
			}
			return ffs;
		}
	} else if (isType(OBJ_TYPE::ARRAY)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			string ffs;
			vector<FFJSON*>& objarr = *(val.array);
			ffs = "[";
			bool matter = false;
			int i = 0;
			string ffjsonstr;
			bool firstTime = false;
			while (i < objarr.size()) {
				uint32_t t = objarr[i] != NULL ? objarr[i]->getType() : NUL;
				if (t != UNDEFINED || (t != NUL&&!objarr[i]->isQType(NONE))) {
					if (t != NUL) {
						if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
						if ((isEFlagSet(B64ENCODE_CHILDREN))&&
							 !isEFlagSet(B64ENCODE_STOP))
							objarr[i]->setEFlag(B64ENCODE_CHILDREN);
						ffjsonstr = objarr[i]->queryString();
					} else {
						ffjsonstr = "";
					}
					if (firstTime)ffs += ',';
					firstTime = true;
					if (ffjsonstr.length() > 0) {
						ffs.append(ffjsonstr);
						matter = true;
					}
				}
				i++;
			}
			if (matter) {
				ffs += ']';
			} else {
				ffs = "";
			};
			return ffs;
		}
	} else if (!isQType(NONE)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else {
		return "";
	}
}

FFJSON* FFJSON::answerObject (
	FFJSON* queryObject, FFJSONPObj* pObj,
	FerryTimeStamp lastUpdateTime, FFJSON* ao
) {
	FFJSON* fp = this;
	if (isLink()) {
		fp = fp->val.fptr;
	}
	FFJSONPObj ffpThisObj;
	ffpThisObj.value = this;
	ffpThisObj.pObj = pObj;
	if (queryObject->isQType(UPDATE)) {
		if (queryObject->isType(UNDEFINED)) {
			if (fp->isType(OBJECT))
				queryObject->init("{}");
			else if (fp->isType(ARRAY))
				queryObject->init("[]");
			else
				queryObject->setQType(QUERY);
		}
	} else if (sm_mUpdateObjs.find(this) != sm_mUpdateObjs.end()) {
		if (queryObject->isType(UNDEFINED)) {
			if (fp->isType(OBJECT))
				queryObject->init("{}");
			else if (fp->isType(ARRAY))
				queryObject->init("[]");
		}
		if (fp->isQType(UPDATE)) {
			if (!queryObject->isQType(DEL)) {
				if (pObj->value->isType(OBJECT)) {
					FFJSON::Iterator itUpdateTime =
						pObj->value->find(*pObj->name);
					++itUpdateTime;
					if (itUpdateTime.getIndex().find("(Time)") == 0) {
						if (((FerryTimeStamp&)(*itUpdateTime)>=lastUpdateTime)) {
							if(queryObject->isType(OBJECT)||queryObject->isType(ARRAY))
								queryObject->setQType(UPDATE);
							else
								queryObject->setQType(QUERY);
						}
					} else {
						if(queryObject->isType(OBJECT)||queryObject->isType(ARRAY))
							queryObject->setQType(UPDATE);
						else
							queryObject->setQType(QUERY);
					}
				}
			}
		} else if (sm_mUpdateObjs[this].size() > 0) {
			if (queryObject->isType(OBJECT)) {
				set<FFJSON::FFJSONIterator>& lsObjs = sm_mUpdateObjs[this];
				set<FFJSON::FFJSONIterator>::iterator i = lsObjs.begin();
				while (i != lsObjs.end()) {
					(*queryObject)[i->m_itMap->first];
					i++;
				}
			} else if (queryObject->isType(ARRAY)) {

			}
		}
	}
	if (queryObject->isQType(DEL)) {
		fp->freeObj();
		setType(NUL);
	} else if (queryObject->isQType(QUERY)) {
		ao = new FFJSON(*this);
	} else if (queryObject->isType(fp->getType())) {
		if (queryObject->isQType(NQUERY)) {
			FFJSON& rao=*ao;
			map<string, FFJSON*>::iterator it = fp->val.pairs->begin();
			while (it != fp->val.pairs->end()) {
				map<string, FFJSON*>::iterator fit =
					queryObject->val.pairs->find(it->first);
				if (fit==queryObject->val.pairs->end()) {
					rao[it->first] = it->second;
				}
				++it;
			}
			return nullptr;
		}
		if (queryObject->isType(OBJECT)) {
			map<string, FFJSON*>::iterator i, j;
			FeaturedMember fmMapSequence, fmOrigMapSequence;
			fmMapSequence = queryObject->
				getFeaturedMember(FM_MAP_SEQUENCE);
			int iMapSeqIndexer = 0, iOrigMapSeqIndexer = 0;
			map<string, FFJSON*>::iterator itEnd;
			if (fmMapSequence.m_pvpsMapSequence) {
				if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->size()) {
					i = (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
				} else {
					i = queryObject->val.pairs->end();
				}
			} else {
				i = queryObject->val.pairs->begin();
			}
			itEnd = queryObject->val.pairs->end();
			j = itEnd;
			if (!queryObject->isQType(UPDATE))
				goto skiporig;
			j = i;
			fmOrigMapSequence = fmMapSequence;
			fmMapSequence = getFeaturedMember(FM_MAP_SEQUENCE);
			if (fmMapSequence.m_pvpsMapSequence) {
				if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->
					 size()) {
					i = (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
				} else {
					i = fp->val.pairs->end();
				}
			} else {
				i = fp->val.pairs->begin();
			}
			itEnd = fp->val.pairs->end();
		  skiporig:
			FeaturedMember fmAOMapSequence;
			while (i != itEnd) {
				map<string, FFJSON*>::iterator k;
				FFJSON* lao = NULL;
				ffpThisObj.name = &i->first;
				FFJSON tempUpdtObj("^");
				if (queryObject->isQType(UPDATE)) {
					if (i->first == j->first) {
						j->second->setQType(UPDATE);
						lao = i->second->answerObject(j->second, &ffpThisObj,
																lastUpdateTime);
					} else {
						lao = i->second->answerObject(&tempUpdtObj,
																&ffpThisObj, lastUpdateTime);
					}
				} else if ((k = fp->val.pairs->find(i->first)) !=
							  fp->val.pairs->end()) {
					lao = k->second->answerObject(i->second, &ffpThisObj,
															lastUpdateTime);
				} else {
					/*FFJSON* nao = new FFJSON(*i->second);
					  if (!nao->isType(UNDEFINED)) {
					  (*this).val.pairs[i->first] = nao;
					  }*/
				}
				if (lao != NULL) {
					if (ao == NULL)ao = new FFJSON(OBJECT);
					fmAOMapSequence = ao->getFeaturedMember
						(FM_MAP_SEQUENCE);
					pair < map<string, FFJSON*>::iterator, bool> prNew =
						ao->val.pairs->
						insert(pair<string, FFJSON*>(i->first, lao));
					if (ao->size < MAX_ORDERED_MEMBERS) {
						fmAOMapSequence.m_pvpsMapSequence->
							push_back(prNew.first);
					} else if (fmAOMapSequence.m_pvpsMapSequence) {
						ao->deleteFeaturedMember(FM_MAP_SEQUENCE);
					}
				}
				if (j != itEnd && i->first == j->first) {
					if (fmOrigMapSequence.m_pvpsMapSequence) {
						if (iOrigMapSeqIndexer < fmOrigMapSequence.
							 m_pvpsMapSequence->size()) {
							j = (*fmOrigMapSequence.m_pvpsMapSequence)
								[iOrigMapSeqIndexer++];
						} else {
							j = queryObject->val.pairs->end();
						}
					} else {
						j++;
					}
				}
				if (fmMapSequence.m_pvpsMapSequence) {
					if (iMapSeqIndexer < fmMapSequence.m_pvpsMapSequence->size()) {
						i = (*fmMapSequence.m_pvpsMapSequence)[iMapSeqIndexer++];
					} else {
						i = itEnd;
					}
				} else {
					i++;
				}
			}
			if (queryObject->isQType(UPDATE) && j != queryObject->val.pairs
				 ->end()) {
				ffl_err(FFJ_MAIN,
						  "QueryObject order didn't match. QueryObject:%s",
						  queryObject->stringify().c_str());
			}
		} else if (queryObject->isType(ARRAY)) {
			if (queryObject->size == size) {
				int i = 0;
				bool matter = false;
				ao = new FFJSON("[]");
				while (i < size) {
					if ((*queryObject->val.array)[i] != NULL
						 || queryObject->isQType(UPDATE)) {
						FFJSON* ffo = NULL;
						string index = to_string(i);
						ffpThisObj.name = &index;
						FFJSON tempUpdtObj("^");
						if ((*queryObject->val.array)[i] == NULL) {
							ffo = (*fp->val.array)[i]->answerObject
								(&tempUpdtObj, &ffpThisObj, lastUpdateTime);
						} else {
							ffo = (*fp->val.array)[i]->answerObject
								((*queryObject->val.array)[i], &ffpThisObj,
								 lastUpdateTime);
						}
						if (ffo) {
							ao->val.array->push_back(ffo);
							matter = true;
						} else {
							ao->val.array->push_back(NULL);
						}
					} else {
						ao->val.array->push_back(NULL);
					}
					i++;
				}
				if (!matter) {
					delete ao;
					ao = NULL;
				}
			}
		} else if (queryObject->isType(STRING) ||
					  queryObject->isType(XML) ||
					  queryObject->isType(BOOL) ||
					  queryObject->isType(NUMBER) ||
					  queryObject->isType(TIME)) {
			freeObj();
			copy(*queryObject);
		} else {
			ao = NULL;
		}
	}
	return ao;
}

//bool FFJSON::isType(uint8_t t) const {
//
//	  return (t == type);
//}

bool FFJSON::isType (OBJ_TYPE t) const {
	return (t == (uint8_t)(flags & 0x000000ff));
}

bool FFJSON::isLink () const {
	OBJ_TYPE t = (OBJ_TYPE)(flags & 0x000000ff);
	return (LINK == t||DLINK == t);
}


//void FFJSON::setType(uint8_t t) {
//
//	  type = t;
//}

void FFJSON::setType (OBJ_TYPE t) {
	flags &= 0xffffff00;
	flags |= t;
}

//uint8_t FFJSON::getType() const {
//
//	  return type;
//}

FFJSON::OBJ_TYPE FFJSON::getType () const {
	uint32_t type = 0xff;
	type &= flags;
	return (OBJ_TYPE) type;
}

//bool FFJSON::isQType(uint8_t t) const {
//
//	  return (t == qtype);
//}

bool FFJSON::isQType (QUERY_TYPE t) const {
	uint32_t qtype = flags;
	qtype &= 0xff00;
	return (t == qtype);
}

//void FFJSON::setQType(uint8_t t) {
//			 
//	  qtype = t;
//}

void FFJSON::setQType (QUERY_TYPE t) {
	flags &= (~0xff00);
	flags |= t;
}

//uint8_t FFJSON::getQType() const {
//
//	  return qtype;
//}

FFJSON::QUERY_TYPE FFJSON::getQType () const {
	uint32_t qtype = flags;
	qtype &= 0xff00;
	return (QUERY_TYPE) qtype;
}

//bool FFJSON::isEFlagSet(int t) const {
//
//	  return (t & etype == t);
//}

bool FFJSON::isEFlagSet (E_FLAGS t) const {
	return (t & flags);
}

//uint8_t FFJSON::getEFlags() const {
//
//	  return this->etype;
//}

FFJSON::E_FLAGS FFJSON::getEFlags () const {
	return (E_FLAGS) (flags & 0x0fff0000);
}

//void FFJSON::setEFlag(int t) {
//		
//	  etype |= t;
//}

void FFJSON::setEFlag (E_FLAGS t) const {
	flags |= t;
}

//void FFJSON::clearEFlag(int t) {
//
//	  etype &= ~t;
//}

void FFJSON::clearEFlag (E_FLAGS t) {
	flags &= ~(t);
}

void FFJSON::erase (string name) {
	FFJSON* fp = this;
	if (isLink())fp= val.fptr;
	if (fp->isType(OBJECT)) {
		vector<map<string, FFJSON*>::iterator>* fmMapSequence=
			fp->getFeaturedMember(FM_MAP_SEQUENCE).m_pvpsMapSequence;
		map<string, FFJSON*>::iterator it= fp->val.pairs->find(name);
		if (it == fp->val.pairs->end())return;
		if (fmMapSequence) {
			fmMapSequence->erase(
				std::find(fmMapSequence->begin(),fmMapSequence->end(),it));
		}
		delete it->second;
		fp->val.pairs->erase(it);
		--size;
	} else if (fp->isType(BIG_OBJECT)) {
		map<string, FFJSON*>::iterator it= fp->val.pairs->find(name);
		if (it == fp->val.pairs->end())return;
		delete it->second;
		fp->val.pairs->erase(it);
		--size;
	}
}

void FFJSON::erase (int index) {
	if (isType(ARRAY)) {
		if (index < size) {
			delete (*val.array)[index];
			(*val.array)[index]= NULL;
		}
	}
}

uint FFJSON::erase (uint start, uint end) {
	if (end>size) {
		end= size;
	}
	if (start>end){
		return 0;
	}
	if (isType(ARRAY)) {
		val.array->erase(val.array->begin()+start, val.array->begin()+end);
		size-=end-start;
		return end-start;
	}
	return 0;
}


void FFJSON::erase (FFJSON* value) {
	if (isType(OBJECT)) {
		map<string, FFJSON*>::iterator i = val.pairs->begin();
		FeaturedMember fmMapSequence = getFeaturedMember(FM_MAP_SEQUENCE);
		while (i != val.pairs->end()) {
			if (i->second == value) {
				if (fmMapSequence.m_pvpsMapSequence) {
					auto e = remove(fmMapSequence.m_pvpsMapSequence->begin(),
										 fmMapSequence.m_pvpsMapSequence->end(), i);
				}
				delete i->second;
				val.pairs->erase(i);
				break;
			}
			i++;
		}
	} else if (isType(ARRAY)) {
		int i = 0;
		while (i < size) {
			if ((*val.array)[i] == value) {
				delete (*val.array)[i];
				(*val.array)[i] = NULL;

				break;
			}
			i++;
		}
	} else if (isType(SET_TYPE)) {
		ffset::iterator it = val.setPtr->find(value);
		if (it!=val.setPtr->end()) {
			delete *it;
			val.setPtr->erase(it);
			--size;
		}
	}
}

bool FFJSON::inherit (FFJSON& rObj, FFJSONPObj* pFPObj) {
	FFJSON* pObj = &rObj;
	if(!pFPObj->name)pFPObj=pFPObj->pObj;
	if (rObj.isLink()) {
		pObj = val.fptr;
	}
	FFJSON& obj = *pObj;
	map<string, int>* m = NULL;
	if (obj.size == 1) {
		//only links are allowed to be inherited
		//so parents should be declared first (:
		FFJSON* arr = NULL;
		if (obj.isType(ARRAY)) {
			if ((*obj.val.array)[0] &&
				 (*obj.val.array)[0]->isLink())
				arr = &obj[0];
			else return false;
		} else if (obj.isType(OBJECT)) {
			if ((*obj.val.pairs)["*"]->isLink())
				arr = &obj["*"];
			else return false;
		} else {
			return false;
		}
		m = new map<string, int>();
		int i = arr->size;
		while (i > 0) {
			i--;
			(*m)[string((const char*) (*arr)[i])] = i;
		}
		if (isType(ARRAY)) {
			i = size;
			while (i > 0) {
				i--;
				(*val.array)[i]->setEFlag(EXT_VIA_PARENT);
				FeaturedMember cFM;
				cFM.tabHead = m;
				(*val.array)[i]->insertFeaturedMember(cFM, FM_TABHEAD);
			}
		} else if (isType(OBJECT)) {
			map<string, FFJSON*>::iterator it = val.pairs->begin();
			while (it != val.pairs->end()) {
				it->second->setEFlag(EXT_VIA_PARENT);
				FeaturedMember cFM;
				cFM.tabHead = m;
				it->second->insertFeaturedMember(cFM, FM_TABHEAD);
				it++;
			}
		} else {
			return false;
		}
		//set "this" as child to the parent
		Link& rLnParent = obj.isType(ARRAY) ?
			*(*obj.val.array)[0]->getFeaturedMember(FM_LINK).link
			: *(*obj.val.pairs)["*"]->getFeaturedMember(FM_LINK).link;
		vector<const string*> path;
		FFJSONPObj* pFPObjTemp = pFPObj;
		bool bParentFound = false;
		while (pFPObjTemp != NULL) {
			if (pFPObjTemp->value->isType(OBJECT)) {
				if (rLnParent.size() && pFPObjTemp->value->val.
					 pairs->find(rLnParent[0]) != pFPObjTemp->
					 value->val.pairs->end()) {
					bParentFound = true;
				}
				path.push_back(pFPObjTemp->name);
			} else if (pFPObjTemp->value->isType(ARRAY)) {
				try {
					path.push_back(pFPObjTemp->name);
					if (pFPObjTemp->value->size > atoi(rLnParent[0].c_str())) {
						bParentFound = true;
					}
				} catch (invalid_argument& ia) {
					//ffl_err(FFJ_MAIN, "array member name is not a number");
				}
			}
			if (bParentFound) {
				FFJSON* pParentRoot = pFPObjTemp->value;
				int iParentLnIndexer = 0;
				do {
					if (pParentRoot->isType(OBJECT)) {
						pParentRoot = (*pParentRoot->val.pairs).
							find(rLnParent[iParentLnIndexer++])
							->second;
					} else if (pParentRoot->isType(ARRAY)) {
						try {
							pParentRoot = (*pParentRoot->val.array)
								[atoi(rLnParent[iParentLnIndexer++].c_str())];
						} catch (Exception e) {
							pParentRoot = NULL;
						}
					} else {
						pParentRoot = NULL;
					}
					if (iParentLnIndexer < rLnParent.size())
						pParentRoot = pFPObj->value->val.pairs->
							at(rLnParent[iParentLnIndexer++]);
				} while (pParentRoot && iParentLnIndexer <
							rLnParent.size());
				if (pParentRoot) {
					FFJSON* pffLink = new FFJSON();
					pffLink->setType(LINK);
					pffLink->val.fptr = this;
					FeaturedMember cFM;
					Link* pLnChild = new Link();
					for (int i = path.size() - 1; i >= 0; i--) {
						pLnChild->push_back(*path[i]);
					}
					cFM.link = pLnChild;
					pffLink->insertFeaturedMember(cFM, FM_LINK);
					if (!pParentRoot->isEFlagSet(HAS_CHILDREN)) {
						pParentRoot->setEFlag(HAS_CHILDREN);
						FeaturedMember fmChildren;
						fmChildren.m_pvChildren = new vector<FFJSON*>();
						pParentRoot->insertFeaturedMember(fmChildren,
																	 FM_CHILDREN);
					}
					vector<FFJSON*>* pvfChildren = pParentRoot->
						getFeaturedMember(FM_CHILDREN).m_pvChildren;
					pvfChildren->push_back(pffLink);
					break;
				} else {
					pFPObjTemp = pFPObjTemp->pObj;
				}
			} else {
				pFPObjTemp = pFPObjTemp->pObj;
			}
		}
	} else if (obj.size > 1) {
		return false;
	}
	FeaturedMember cFM;
	cFM.m_pParent = pObj;
	setEFlag(EXTENDED);
	insertFeaturedMember(cFM, FM_PARENT);
	cFM.tabHead = m;
	setEFlag(EXT_VIA_PARENT);
	insertFeaturedMember(cFM, FM_TABHEAD);
	return true;
}

FFJSON::Iterator FFJSON::begin () {
	FFJSON* fp = this;
	if (isLink()) {
		return val.fptr->begin();
	}
	Iterator i(*fp);

	return i;
}

FFJSON::Iterator FFJSON::end () {
	FFJSON* fp = this;
	if (isLink()) {
		return val.fptr->end();
	}
	Iterator i(*fp, true);

	return i;
}

FFJSON::Iterator::Iterator () {
	type = NUL;
	m_uContainerPs.m_pMap=NULL;
}

FFJSON::Iterator::Iterator (const Iterator& orig) {
	copy(orig);
}

FFJSON::Iterator::Iterator (const FFJSON& orig, bool end) {
	init(orig, end);
}

FFJSON::Iterator::Iterator (map<string, FFJSON*>::iterator pi) {
	ui.pi = pi;
	type = BIG_OBJECT;
}

FFJSON::Iterator::Iterator (vector<FFJSON*>::iterator ai) {
	ui.ai = ai;
	type = ARRAY;
}

FFJSON::Iterator::Iterator (
	vector<map<string, FFJSON*>::iterator >::iterator pai,
	vector<map<string,FFJSON*>::iterator>* pMapItVec
) {
	this->ui.pai= pai;
	type= OBJECT;
	m_uContainerPs.m_pMapVector= pMapItVec;
}

FFJSON::Iterator::~Iterator () {

}

void FFJSON::Iterator::copy (const Iterator& i) {
	type= i.type;
	ui= i.ui;
	m_uContainerPs= i.m_uContainerPs;
}

void FFJSON::Iterator::init (const FFJSON& orig, bool end) {
	switch (orig.getType()) {
		case ARRAY: {
			type= ARRAY;
			ui.ai= end? orig.val.array->end() : orig.val.array->begin();
			m_uContainerPs.m_pVector= orig.val.array;
			break;
		}
		case OBJECT: {
			FeaturedMember fm= orig.getFeaturedMember(FM_MAP_SEQUENCE);
			type= BIG_OBJECT;
			if (fm.m_pvpsMapSequence != NULL) {
				ui.pai= end? fm.m_pvpsMapSequence->end() :
					fm.m_pvpsMapSequence->begin();
				type= OBJECT;
				m_uContainerPs.m_pMapVector= fm.m_pvpsMapSequence;
			} else {
				ui.pi= end? orig.val.pairs->end() : orig.val.pairs->begin();
				m_uContainerPs.m_pMap= orig.val.pairs;
			}
			break;			
		}
		case SET_TYPE: {
			type= SET_TYPE;
			ui.si= end? orig.val.setPtr->end():orig.val.setPtr->begin();
			m_uContainerPs.m_pSet= orig.val.setPtr;
			break;
		}
		default:
			type= NUL;
	}
}

FFJSON::Iterator& FFJSON::Iterator::operator = (const Iterator& i) {
	copy(i);
	return *this;
}

string FFJSON::Iterator::getIndex () {
	switch (type) {
		case OBJECT:
			return (*ui.pai)->first;
		case BIG_OBJECT:
			return ui.pi->first;
		default:
			ffl_err (
				FFJ_MAIN,
				"Index from non object type container is being retrieved."
			);
			return string ();
				{}
	}
}

int FFJSON::Iterator::getIndex (const FFJSON& rCurArray) {
	if (type == ARRAY) {
		return ui.ai - rCurArray.val.array->begin();
	} else {
		ffl_debug(FFJ_MAIN,
					 "Index from non array type container is being retrieved.");
		return 0;
	}
}

FFJSON& FFJSON::Iterator::operator * () {
	if (type == BIG_OBJECT) {
		return *(ui.pi->second);
	} else if (type == OBJECT) {
		return *(*ui.pai)->second;
	} else if (type == ARRAY) {
		return **ui.ai;
	} else if (type == SET_TYPE) {
		return **ui.si;
	} else {
		return **ui.ai;
	}
}

FFJSON* FFJSON::Iterator::operator -> () {
	if (type == BIG_OBJECT) {
		return ui.pi->second;
	} else if (type == OBJECT) {
		return (*ui.pai)->second;
	} else if (type == ARRAY) {
		return &(**(ui.ai));
	} else if (type == SET_TYPE) {
		return &(**(ui.si));
	} else {
		return NULL;
	}
}

FFJSON::Iterator& FFJSON::Iterator::operator ++ () {
	if (type == BIG_OBJECT) {
		++ui.pi;
		while (ui.pi != m_uContainerPs.m_pMap->end() &&
				 ui.pi->second->isEFlagSet(COMMENT)) {
			++ui.pi;
		}
	} else if (type == OBJECT) {
		++ui.pai;
		while (ui.pai != m_uContainerPs.m_pMapVector->end() &&
				 (*ui.pai)->second->isEFlagSet(COMMENT)) {
			++ui.pai;
		}
	} else if (type == ARRAY) {
		++ui.ai;
	} else if (type == SET_TYPE) {
		++ui.si;
	}
	return *this;
}

FFJSON::Iterator FFJSON::Iterator::operator ++ (int) {
	FFJSON::Iterator tmp(*this);
	operator ++ ();
	return tmp;
}

FFJSON::Iterator& FFJSON::Iterator::operator -- () {
	if (type == BIG_OBJECT) {
		ui.pi--;
		while (ui.pi->second->isEFlagSet(COMMENT)) {
			--ui.pi;
		}
	} else if (type == OBJECT) {
		--ui.pai;
		while ((*ui.pai)->second->isEFlagSet(COMMENT)) {
			--ui.pai;
		}
	} else if (type == ARRAY) {
		--ui.ai;
	} else if (type == SET_TYPE) {
		--ui.si;
	}
	return *this;
}

FFJSON::Iterator FFJSON::Iterator::operator -- (int) {
	FFJSON::Iterator tmp(*this);
	operator--();

	return tmp;
}

bool FFJSON::Iterator::operator == (const Iterator & i) {
	if (type == i.type) {
		if (type == OBJECT) {
			return (ui.pai == i.ui.pai);
		} else if (type == BIG_OBJECT) {
			return (ui.pi == i.ui.pi);
		} else if (type == ARRAY) {
			return ui.ai == i.ui.ai;
		} else if (type == SET_TYPE) {
			return ui.si == i.ui.si;
		} else if (type == NUL) {
			return true;
		}
	}
	return false;
}

bool FFJSON::Iterator::operator != (const Iterator & i) {
	return !operator==(i);
}

FFJSON::Iterator::operator const char* () {
	if (type == OBJECT) {
		return (*ui.pai)->first.c_str();
	}
	return nullptr;
}

FFJSON::Iterator FFJSON::find (const string& key) {
	switch (getType()) {
		case OBJECT: {
			const char lc = key.back();
			ffmap::iterator itMap;
			if (lc=='*') {
				string lk = key.substr(0,key.length()-1);
				itMap = val.pairs->lower_bound(lk);
				return Iterator(itMap);
			}
			itMap = val.pairs->find(key);
			FeaturedMember fm = getFeaturedMember(FM_MAP_SEQUENCE);
			if (fm.m_pvpsMapSequence) {
				vector<map<string, FFJSON*>::iterator>::iterator itVecMap =
					fm.m_pvpsMapSequence->begin();
				vector<map<string, FFJSON*>::iterator>::iterator itVecMapEnd =
					fm.m_pvpsMapSequence->end();
				while (itVecMap != itVecMapEnd) {
					if (*itVecMap == itMap) {
						return Iterator(itVecMap,fm.m_pvpsMapSequence);
					}
					++itVecMap;
				}
				return Iterator(itVecMap,fm.m_pvpsMapSequence);
			} else {
				return Iterator(itMap);
			}
			break;
		}
		case BIG_OBJECT: {
			const char lc = key.back();
			ffmap::iterator itMap;
			if (lc=='*') {
				string lk = key.substr(0,key.length()-1);
				itMap = val.pairs->lower_bound(lk);
				return Iterator(itMap);
			}
			itMap = val.pairs->find(key);
			return Iterator(itMap);
		}
		case ARRAY: {
			uint ikey = atoi(key.c_str());
			vector<FFJSON*>::iterator itVec;
			itVec = val.array->begin()+ikey;
			return Iterator(itVec);
		}
		case LINK:
		case DLINK:
			return val.fptr->find(key);
		default:
			return Iterator(*this, true);
	}
	return Iterator();
}

FFJSON::FFJSONPrettyPrintPObj::FFJSONPrettyPrintPObj (
	map<const string*, const string*>* m_mpDeps,
	list<string>* m_lsFFPairLst,
	map<string*, const string*>* m_mpMemKeyFFPairMap,
	map<const string*, list<string>::iterator>* pKeyPrettyStringItMap
) : m_mpDeps(m_mpDeps), m_lsFFPairLst(m_lsFFPairLst),
	 m_mpMemKeyFFPairMap(m_mpMemKeyFFPairMap),
	 m_pKeyPrettyStringItMap(pKeyPrettyStringItMap)
{};

void FFJSON::headTheHeader (FFJSONPrettyPrintPObj & lfpo) {
	list<string>::iterator itFFPL = lfpo.m_lsFFPairLst->begin();
	markTheNameIfExtended(&lfpo);
	while (itFFPL != lfpo.m_lsFFPairLst->end()) {
		list<string>::iterator itFFPLNext = itFFPL;
		itFFPLNext++;
		const string* key = (*lfpo.m_mpMemKeyFFPairMap)[&(*itFFPL)];
		if ((*lfpo.m_mpDeps).find(key) != lfpo.m_mpDeps->end()) {
			const string* pDepKey = (*lfpo.m_mpDeps)[(*lfpo.m_mpMemKeyFFPairMap)
																  [&(*itFFPL)]];
			list<string>::iterator itPrettyParent =
				(*lfpo.m_pKeyPrettyStringItMap)[pDepKey];
			itPrettyParent++;
			lfpo.m_lsFFPairLst->splice(itPrettyParent, *lfpo.m_lsFFPairLst,
												itFFPL);
		}
		itFFPL = itFFPLNext;
	}
}

ostream& operator << (ostream& out, const FFJSON& f) {
	out << f.prettyString();
	return out;
}

bool operator < (const FFJSON& lhs, const FFJSON& rhs) {
	ffl_debug(FFJ_MAIN, "< operator");
	if (!lhs.isType(rhs.getType())) {
		if (lhs.isType(FFJSON::LINK)) {
			return lhs.val.fptr < &rhs;
		} else if (rhs.isType(FFJSON::LINK)) {
			return rhs.val.fptr < &lhs;
		}
		return lhs.getType() < rhs.getType();
	} else {
		switch(lhs.getType()) {
			case FFJSON::NUMBER:
				return lhs.val.number < rhs.val.number;
			case FFJSON::TIME:
				return *lhs.val.m_pFerryTimeStamp < *rhs.val.m_pFerryTimeStamp;
			case FFJSON::STRING:
				return strcmp(lhs.val.string, rhs.val.string)<0;
			default:
				return (void*)lhs.val.fptr < (void*)rhs.val.fptr;
		}
	}
	return (const void*)&lhs<(const void*)&rhs;
}

bool operator == (const FFJSON& lhs, const FFJSON& rhs) {
	ffl_debug(FFJ_MAIN, "== operator");
	if (!lhs.isType(rhs.getType())) {
		if (lhs.isType(FFJSON::LINK)) {
			return lhs.val.fptr == &rhs;
		} else if (rhs.isType(FFJSON::LINK)) {
			return rhs.val.fptr == &lhs;
		}
		return false;
	}
	switch(lhs.getType()) {
		case FFJSON::NUMBER:
			return lhs.val.number == rhs.val.number;
		case FFJSON::TIME:
			return *lhs.val.m_pFerryTimeStamp == *rhs.val.m_pFerryTimeStamp;
		case FFJSON::STRING:
		case FFJSON::XML:
			if (*lhs.val.string == *rhs.val.string)
				return true;
			else
				return false;
		case FFJSON::OBJECT:
		case FFJSON::BIG_OBJECT: {
			if (lhs.size != rhs.size)
				return false;
			ffmap::iterator it = lhs.val.pairs->begin();
			while (it!=lhs.val.pairs->end()) {
				ffmap::iterator rit = rhs.val.pairs->find(it->first);
				if (rit==rhs.val.pairs->end())
					return false;
				if (*rit->second!=*it->second)
					return false;
				++it;
			}
			return true;
		}
		case FFJSON::SET_TYPE: {
			if (lhs.size != rhs.size)
				return false;
			ffset::iterator lit = lhs.val.setPtr->begin();
			ffset::iterator rit = rhs.val.setPtr->begin();
			while (lit!=lhs.val.setPtr->end()) {
				if (**lit!=**rit)
					return false;
				++rit;++lit;
			}
			return true;
		}
		case FFJSON::ARRAY:
			if (lhs.size != rhs.size)
				return false;
			for (int i=0; i<lhs.size;++i)
				if (*(*rhs.val.array)[i]!=*(*lhs.val.array)[i])
					return false;
			return true;
		case FFJSON::LINK:
		case FFJSON::DLINK:
			return *lhs.val.fptr == *rhs.val.fptr;
		default:
			return (void*)lhs.val.fptr == (void*)rhs.val.fptr;
	}

	return (const void*)&lhs<(const void*)&rhs;
}

FFJSON* FFJSON::MarkAsUpdatable(string& link, const FFJSON& rParent) {
	if (rParent.isType(OBJECT) || rParent.isType(ARRAY)) {
		FFJSON* pOrigParent = const_cast<FFJSON*> (&rParent);
		FFJSON* pParent = pOrigParent;
		vector<string> prop;
		explode(".", link, prop);
		for (int i = 0; i < prop.size(); i++) {
			if (pParent->isType(OBJECT) && pParent->val.pairs->find(prop[i]) ==
				 pParent->val.pairs->end()) {
				return NULL;
			} else if (pParent->isType(ARRAY) && atoi(prop[i].c_str()) >= pParent->size) {
				return NULL;
			}
			pParent = &((*pParent)[prop[i]]);
		}
		pParent = pOrigParent;
		for (int i = 0; i < prop.size(); i++) {
			FFJSONIterator UpdatablePair;
			if (pParent->isType(OBJECT)) {
				UpdatablePair.m_itMap = pParent->val.pairs->find(prop[i]);
				sm_mUpdateObjs[pParent].insert(UpdatablePair);
			} else if (pParent->isType(ARRAY)) {
				UpdatablePair.m_uiIndex = atoi(prop[i].c_str());
				sm_mUpdateObjs[pParent].insert(
					UpdatablePair);
			}
			pParent = &(*pParent)[prop[i]];
		}
		if (sm_mUpdateObjs.find(pParent) == sm_mUpdateObjs.end()) {
			sm_mUpdateObjs[pParent];
		}
		pParent->setQType(UPDATE);
		return pParent;
	}
	return NULL;
}

FFJSON* FFJSON::UnMarkUpdatable(string& link, const FFJSON& rParent) {
	FFJSON* pParent = const_cast<FFJSON*> (&rParent);
	if (rParent.isType(OBJECT) || rParent.isType(ARRAY)) {
		map<FFJSON*, FFJSON*> rOrigParent;
		vector<string> prop;
		explode(".", link, prop);
		for (int i = 0; i < prop.size(); i++) {
			if (pParent->isType(OBJECT) && pParent->val.pairs->find(prop[i]) ==
				 pParent->val.pairs->end()) {
				return NULL;
			} else if (pParent->isType(ARRAY) && atoi(prop[i].c_str()) >=
						  pParent->size) {
				return NULL;
			}
			rOrigParent[(*pParent)[prop[i]]] = pParent;
			pParent = (*pParent)[prop[i]];
		}
		pParent->setQType(NONE);
		if (sm_mUpdateObjs.find(pParent) != sm_mUpdateObjs.end() &&
			 sm_mUpdateObjs[pParent].size() == 0) {
			string sObjName;
			for (int i = prop.size() - 1; i >= 0; i++) {
				if (pParent->isType(OBJECT))
					sm_mUpdateObjs[pParent].erase(pParent->val.pairs->
															find(sObjName));
				else if (pParent->isType(ARRAY) && sObjName.length() > 0)
					sm_mUpdateObjs[pParent].erase(atoi(sObjName.c_str()));
				if (rParent.getQType() == UPDATE) {
					return pParent;
				}
				if (sm_mUpdateObjs[pParent].size() == 0) {
					sm_mUpdateObjs.erase(pParent);
					sObjName = prop[i];
				} else {
					return pParent;
				}
				pParent = rOrigParent[pParent];
			}
			if (pParent->getQType() == UPDATE) {
				return pParent;
			}
			sm_mUpdateObjs.erase(pParent);
			return pParent;
		}
	}
	return NULL;
}

FFJSON::LinkNRef FFJSON::GetLinkString (FFJSONPObj* pObj) {
	LinkNRef lnr;
	while (pObj != NULL) {
		lnr.m_sLink= *pObj->name + "." + lnr.m_sLink;
		lnr.m_pRef= pObj->value;
		pObj= pObj->pObj;
	}
	return lnr;
}

int FFJSON::save (
	bool json, bool printComments, unsigned int indent,
	FFJSONPrettyPrintPObj* pObj, bool printFilePath, bool save
) const {
	string sOut= json?stringify(json):
		prettyString(json, printComments, 0, pObj, false, true);
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		const char* fn= getFeaturedMember(FM_FILE).m_sFileName;
		ofstream ofs(fn, ios::out|ios::trunc);
		if (ofs.is_open()) {
			ofs << sOut;
			ofs.close();
			setEFlag(FILE);
			return sOut.length();
		} else {
			ffl_warn(FFJ_MAIN, "couldn't create %s", fn);
		}
		return -2;
	}
	return -1;
}

bool FFPtrCmp::operator() (const FFJSON* a, const FFJSON* b) const {
	return *a < *b;
}
