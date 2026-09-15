/* 
 * File:	  Txj_.cpp
 * Author: Gowtham Kudupudi
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

#include "Texject.h"

using namespace std;

const char Txj_::OBJ_STR[15][15]= {
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
	"OBJ",
	"ORDERED_OBJ", //number of members are greater than MAX_ORDERED_MEMBERS; Its
	"LINK",		  //used only by Iterator.
	"DLINK",
	"NUL"
};
map<Txj_*, set<Txj_::TxjIterator> > Txj_::sm_mUpdateObjs;
map<const Txj_*, shared_mutex> Txj_::MtxMap;
shared_mutex Txj_::MtxMapMtx;

void iterSeq (vector<ffmap::iterator>* vecPtr, ffmap::iterator& i, int& ind,
				  ffmap& objmap) {
	if (vecPtr) {
		if (ind<vecPtr->size())
			i= (*vecPtr)[ind++];
		else
			i= objmap.end();
	} else {
		++i;
	}
}

Txj_::Txj_ () {
	//	  type = UNDEFINED;
	//	  qtype = NONE;
	//	  etype = ENONE;
	flags= 0;
	size= 0;
	val.number= 0;
	val.boolean= false;
}

Txj_::Txj_ (OBJ_TYPE t) {
	// type= UNDEFINED;
	// qtype= NONE;
	// etype= ENONE;
	flags= 0;
	FeaturedMember fmMapSequence;
	switch (t) {
	case ORDERED_OBJ:
		setType(ORDERED_OBJ);
		fmMapSequence.m_pvpsMapSequence=
			new vector<ffmap::iterator>();
		insertFeaturedMember(fmMapSequence, FM_MAP_SEQUENCE);
		goto insertPairs;
	case OBJ:
		setType(OBJ);
	insertPairs:
		val.pairs= new ffmap(); break;
	case ARRAY:
		setType(ARRAY);
		val.array= new vector<Txj_*>(); break;
	case SET_TYPE:
	case STRING:
		setType(STRING); break;
	case XML:
		setType(XML); break;
	case NUMBER:
	case TIME:
		setType(TIME); break;
		val.m_pFerryTimeStamp= new FerryTimeStamp(); break;
	case BOOL:
		setType(BOOL); break;
	case NUL:
		setType(NUL); break;
	default:
		setType(UNDEFINED); break;
	}
}

Txj_::Txj_ (const Txj_& orig, COPY_FLAGS cf, TxjPObj* pObj) {
	copy(orig, cf, pObj);
}

Txj_::Txj_ (const string& ffjson, int* ci, int indent, Txj_::TxjPObj* pObj)
	: size(0), flags(0) {
	//Txj_::TxjPObj* pObj) {
	init(ffjson, ci, indent, pObj);
}

void Txj_::copy (const Txj_& orig, COPY_FLAGS cf, TxjPObj* pObj) {
	if (!(isType(OBJ) || isType(ORDERED_OBJ) || isType(ARRAY))) {
		lock();
		freeObj();
		flags= 0;
		unlock();
	}
	orig.lockShared();
	OBJ_TYPE origType= orig.getType();
	switch (origType) {
	case NUMBER:
		lock();
		val.number= orig.val.number;
		if (orig.isEFlagSet(PRECISION)) {
			setEFlag(PRECISION);
			FeaturedMember cFM= orig.getFeaturedMember(FM_PRECISION);
			insertFeaturedMember(cFM, FM_PRECISION);
		}
		setType(origType);
		size= orig.size;
		unlock();
		break;
	case STRING: {
		lock();
		val.str= new char[orig.size+1];
		memcpy(val.str, orig.val.str, orig.size);
		val.str[orig.size]= '\0';
		size= orig.size;
		if (cf & COPY_EFLAGS) {
			setEFlag(orig.getEFlags());
		}
		//FeaturedMember fmWidth;
		//fmWidth.width= orig.getFeaturedMember(FM_WIDTH).width;
		//insertFeaturedMember(fmWidth, FM_WIDTH);
		setType(origType);
		size= orig.size;
		unlock();
		break;
	}
	case XML:
		lock();
		val.str= new char[orig.size+1];
		memcpy(val.str, orig.val.str, orig.size);
		val.str[orig.size]= '\0';
		size= orig.size;
		if (cf & COPY_EFLAGS) {
			setEFlag(orig.getEFlags());
		}
		setType(origType);
		size= orig.size;
		unlock();
		break;
	case BOOL:
		lock();
		val.boolean= orig.val.boolean;
		setType(origType);
		size= orig.size;
		unlock();
		break;
	case BINARY:
		lock();
		val.vptr= (uint8_t*)malloc(orig.size);
		memcpy(val.vptr, orig.val.vptr, orig.size);
		size= orig.size;
		setType(origType);
		unlock();
		break;
	case ORDERED_OBJ:
		lock();
		if (!val.pairs) {
			setType(origType);
			FeaturedMember fm;
			fm.m_pvpsMapSequence= new vector<ffmap::iterator>();
			insertFeaturedMember(fm, FM_MAP_SEQUENCE);
		}
		goto copyObj;
	case OBJ: {
		lock();
	  copyObj:
		if (!val.pairs) {
			setType(origType);
			val.pairs= new ffmap();
			size= 0;
		}
		FeaturedMember fm= getFeaturedMember(FM_MAP_SEQUENCE);
		ffmap::iterator i;
		ffmap& objmap= *orig.val.pairs;
		FeaturedMember fmOrigMapSequence=
			orig.getFeaturedMember(FM_MAP_SEQUENCE);
		int iMapSeqIndexer= 0;
		vector<ffmap::iterator>* itVecPtr= fmOrigMapSequence.m_pvpsMapSequence;
		if (itVecPtr) iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
		else i= objmap.begin();			
		TxjPObj pLObj;
		pLObj.pObj= pObj;
		pLObj.value= this;
		while (i!=orig.val.pairs->end()) {
			Txj_* fo= NULL;
			pLObj.name= &i->first;
			ffmap::iterator ii= val.pairs->find(i->first);
			if (ii!=val.pairs->end() && ((cf&COPY_SHALLOW)!=COPY_SHALLOW)) {
				ii->second->copy(*i->second, cf, &pLObj);
			} else {
				if (((cf&COPY_SHALLOW)==COPY_SHALLOW)) {
					delete ii->second;
					fo= new Txj_();
					*fo= &(*i->second);
				} else {
					fo= new Txj_(*i->second, cf, &pLObj);
				}//pair<ffmap::iterator, bool> prNew= val.pairs->insert(pair<string, Txj_*>(i->first, new Txj_(*i->second, cf, &pLObj)));
					
			}
			if (fo && ((cf==COPY_QUERIES && !fo->isQType(QUERY_TYPE::NONE))
						  || !fo->isType(UNDEFINED))) {
				pair<ffmap::iterator, bool> prNew= val.
					pairs->insert(pair<string, Txj_*>(i->first, fo));
				++size;
				if (fm.m_pvpsMapSequence)
					fm.m_pvpsMapSequence->push_back(prNew.first);
			} else {
				delete fo;
			}
			iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
		}
		if (val.pairs->size()==0) {
			delete val.pairs;
			val.pairs= NULL;
			setType(UNDEFINED);
		};
		unlock();
		break;
	}
	case ARRAY: {
		int i= 0;
		bool matter= false;
		TxjPObj pLObj;
		pLObj.pObj= pObj;
		pLObj.value= this;
		if (val.array==nullptr) {
			lock();
			size= 0;
			val.array= new vector<Txj_*>();
			setType(ARRAY);
			unlock();
		}
		lockShared();
		while (i<orig.val.array->size()) {
			Txj_* fo= NULL;
			string index= to_string(i);
			pLObj.name= &index;
			if ((*orig.val.array)[i]!=NULL) {
				unlockShared();
				(*this)[i].copy(*(*orig.val.array)[i]);
				lockShared();
			}
			++i;
			matter= true;
		}
		if (orig.isEFlagSet(EXTENDED)) {
			map<string, int>* m= new map<string, int>();
			FeaturedMember cFM= orig.getFeaturedMember(FM_TABHEAD);
			(*m)= *cFM.tabHead;
			cFM= orig.getFeaturedMember(FM_PARENT);
			Txj_* mp= new Txj_(*cFM.m_pParent);
			cFM.m_pParent= mp;
			setEFlag(EXTENDED);
			insertFeaturedMember(cFM, FM_PARENT);
			cFM.tabHead= m;
			insertFeaturedMember(cFM, FM_TABHEAD);
			if (mp->isType(ARRAY)) {
				for (int l=0; l< size; ++l) {
					Txj_* pT= (*val.array)[l];
					pT->setEFlag(EXT_VIA_PARENT);
					pT->insertFeaturedMember(cFM, FM_TABHEAD);
				}
			}
		}
		unlockShared();
		if (!matter) {
			lock();
			freeObj();
			unlock();
		}
		break;
	}
	case SET_TYPE: {
		lock();
		size=0;
		val.setPtr= new ffset();
		setType(origType);
		for (Txj_* fp : *orig.val.setPtr) {
			Txj_* newcopy= new Txj_(*fp);
			if (val.setPtr->insert(newcopy).second)
				++size;
			else
				delete newcopy;
		}
		unlock();
		break;
	}
	case LINK: {
		lock();
		vector<string>* ln=
			new vector<string>(*orig.getFeaturedMember(FM_LINK).link);
		FeaturedMember fm;
		fm.link= ln;
		setType(origType);
		insertFeaturedMember(fm, FM_LINK);
		val.fptr= orig.val.fptr;
		size= orig.size;
		unlock();
		break;
	}
	case TIME:
		lock();
		val.m_pFerryTimeStamp=
			new FerryTimeStamp(*orig.val.m_pFerryTimeStamp);
		setType(origType);
		size= orig.size;
		unlock();
		break;
	case NUL:
		lock();
		setType(NUL);
		size= 0;
		val.boolean= false;
		unlock();
		break;
	default:
		lock();
		if ((cf==COPY_QUERIES&&!isQType(QUERY_TYPE::NONE)) &&
			 isType(UNDEFINED)) {
			setQType(orig.getQType());
		} else {
			setType(UNDEFINED);
			val.boolean= false;
		}
		unlock();
		break;
	}
	orig.unlockShared();
	// if (orig.isEFlagSet(EXTENDED) && !isType(STRING)) {
	// 	lock();
	// 	Txj_* pOrigParent= orig.getFeaturedMember(FM_PARENT).m_pParent;
	// 	setEFlag(EXTENDED);
	// 	FeaturedMember fm;
	// 	fm.m_pParent= new Txj_(*pOrigParent, COPY_ALL, pObj);
	// 	insertFeaturedMember(fm, FM_PARENT);
	// 	if (orig.isEFlagSet(EXT_VIA_PARENT)) {
	// 		map<string, int>* pOrigTabHead= orig.getFeaturedMember(FM_TABHEAD).
	// 			tabHead;
	// 		map<string, int>* pTabHead= new map<string, int>(*pOrigTabHead);
	// 		setEFlag(EXT_VIA_PARENT);
	// 		FeaturedMember fm;
	// 		fm.tabHead= pTabHead;
	// 		insertFeaturedMember(fm, FM_TABHEAD);
	// 		if (isType(ARRAY)) {
	// 			vector<Txj_*>& vElems= *val.array;
	// 			for (int i= 0; i<size; ++i) {
	// 				vElems[i]->setEFlag(EXT_VIA_PARENT);
	// 				vElems[i]->insertFeaturedMember(fm, FM_TABHEAD);
	// 			}
	// 		} else if (isType(OBJ)) {
	// 			ffmap::iterator itPairs= val.pairs->begin();
	// 			while (itPairs!=val.pairs->end()) {
	// 				itPairs->second->setEFlag(EXT_VIA_PARENT);
	// 				itPairs->second->insertFeaturedMember(fm, FM_TABHEAD);
	// 				itPairs++;
	// 			}
	// 		}
	// 	}
	// 	Link* linkToParent= NULL;
	// 	if (fm.m_pParent->isType(ARRAY) && fm.m_pParent->size==1) {
	// 		linkToParent= (*fm.m_pParent->val.array)[0]->getFeaturedMember(
	// 			FM_LINK).link;
	// 	} else if (fm.m_pParent->isType(OBJ) && fm.m_pParent->size==1) {
	// 		linkToParent= (*fm.m_pParent->val.pairs)["*"]->getFeaturedMember(
	// 			FM_LINK).link;
	// 	} else {
	// 		flErr(TXJ_MAIN, "Invalid parent size. Not 1.");
	// 	}
	// 	//set "this" as child to the parent
	// 	Link& rLnParent= *linkToParent;
	// 	vector<const string*> path;
	// 	TxjPObj* pFPObjTemp= pObj;
	// 	bool bParentFound= false;
	// 	while (pFPObjTemp!=NULL) {
	// 		if (pFPObjTemp->value->isType(OBJ)) {
	// 			if (rLnParent.size() && pFPObjTemp->value->val.
	// 				 pairs->find(rLnParent[0])!=pFPObjTemp->
	// 				 value->val.pairs->end()) {
	// 				bParentFound= true;
	// 			}
	// 			path.push_back(pFPObjTemp->name);
	// 		} else if (pFPObjTemp->value->isType(ARRAY)) {
	// 			try {
	// 				if (pFPObjTemp->value->size>atoi(pFPObjTemp->name->c_str())) {
	// 					bParentFound= true;
	// 				}
	// 				path.push_back(pFPObjTemp->name);
	// 			} catch (Exception e) {
	// 				flErr(TXJ_MAIN, "array member name is not a number");
	// 			}
	// 		}
	// 		if (bParentFound) {
	// 			Txj_* pParentRoot= pFPObjTemp->value;
	// 			int iParentLnIndexer= 0;
	// 			do {
	// 				if (pParentRoot->isType(OBJ)) {
	// 					pParentRoot= (*pParentRoot->val.pairs).
	// 						find(rLnParent[iParentLnIndexer++])
	// 						->second;
	// 				} else if (pParentRoot->isType(ARRAY)) {
	// 					try {
	// 						pParentRoot= (*pParentRoot->val.array)
	// 							[atoi(rLnParent[iParentLnIndexer++].c_str())];
	// 					} catch (Exception e) {
	// 						pParentRoot= NULL;
	// 					}
	// 				} else {
	// 					pParentRoot= NULL;
	// 				}
	// 				if (iParentLnIndexer<rLnParent.size())
	// 					pParentRoot= pObj->value->val.pairs->
	// 						at(rLnParent[iParentLnIndexer++]);
	// 			} while (pParentRoot && iParentLnIndexer <
	// 						rLnParent.size());
	// 			if (pParentRoot) {
	// 				Txj_* pffLink= new Txj_();
	// 				pffLink->setType(LINK);
	// 				pffLink->val.fptr= this;
	// 				FeaturedMember cFM;
	// 				Link* pLnChild= new Link();
	// 				for (int i= path.size()-1; i>=0; i--) {
	// 					pLnChild->push_back(*path[i]);
	// 				}
	// 				cFM.link= pLnChild;
	// 				pffLink->insertFeaturedMember(cFM, FM_LINK);
	// 				if (!pParentRoot->isEFlagSet(HAS_CHILDREN)) {
	// 					pParentRoot->setEFlag(HAS_CHILDREN);
	// 					FeaturedMember fmChildren;
	// 					fmChildren.m_pvChildren= new vector<Txj_*>();
	// 					pParentRoot->insertFeaturedMember(fmChildren,
	// 																 FM_CHILDREN);
	// 				}
	// 				vector<Txj_*>* pvfChildren= pParentRoot->
	// 					getFeaturedMember(FM_CHILDREN).m_pvChildren;
	// 				pvfChildren->push_back(pffLink);
	// 				break;
	// 			} else {
	// 				pFPObjTemp= pFPObjTemp->pObj;
	// 			}
	// 		} else {
	// 			pFPObjTemp= pFPObjTemp->pObj;
	// 		}
	// 	}
	// 	unlock();
	// }
}

inline bool Txj_::isWhiteSpace(char c) {
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

inline bool Txj_::isTerminatingChar(char c) {
	switch (c) {
	case '\0':
	case ',':
	case '}':
	case ']':
		return true;
	default:
		return false;}
}

inline bool Txj_::isInitializingChar(char c) {
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

void Txj_::init (
	const string& txj, int* ci, int indent, TxjPObj* pObj
) {
	lock();
	if (!isType(UNDEFINED)) {
		freeObj();
	}
	int iii= 0;
	int& i= (ci==nullptr)? iii : *ci;
	int j= txj.length();
	FeaturedMember fmMulLnBuf;
	fmMulLnBuf.m_psMultiLnBuffer= NULL;
	TxjPObj ffpo;
	int parseType= UNDEFINED;
	int objIdNail= i;
	int nind= getIndent(txj.c_str(), &i, indent);
	bool comment= false, gotoObjBackyard= false;
	FeaturedMember fmMapSequence;
	string buf;
	ffpo.value= this;
	ffpo.pObj= pObj;
	string arrInd= "0";
	while (i<j) {
		char txji= txj[i];
		switch (txji) {
		case '[':
			ffpo.name= &arrInd;
			nind= getIndent(txj.c_str(), &i, indent);
			goto braceCommon;
		case '{':
		braceCommon:
			++i;
			ffpo.value= this;
			ffpo.pObj= pObj;
			while (1) {
				ffmap::iterator prNew;
				bool arrEnd= false; ffpo.name= &arrInd;
				Txj_* obj= new Txj_(txj, &i, nind, &ffpo);
				if (i>=j) {
					flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
					unlock();return;
				}
				while (1) {
					switch (txj[i]) {
					case ',': break;
					case ']':
					case '}': arrEnd=true; break;
					default: ++i; continue;
					} break;
				} ++i;
				const string& objId= *ffpo.name;
			  memobtained:
				if (txji=='[') {
					if (isType(ORDERED_OBJ)) {
						goto ObjIns;
					  SeqIns:
						if (objId[0]=='#') {
							comment= true;
						}
						if (
							objId[0]=='(' && objId[1]=='T' && objId[2]=='i' &&
							objId[3]=='m' && objId[4]=='e' && objId[5]==')'
						) {
							LinkNRef lnr= GetLinkString(pObj);
							string sLink= lnr.m_sLink +
								(*fmMapSequence.m_pvpsMapSequence)[size-2]->first;
							Txj_* pOverLooker=
								MarkAsUpdatable(sLink, *(lnr.m_pRef?lnr.m_pRef:this));
							FeaturedMember fm;
							fm.m_pTimeStamp= obj->val.m_pFerryTimeStamp;
							pOverLooker->insertFeaturedMember(
								fm, FM_UPDATE_TIMESTAMP);
						} else {
							ffpo.m_pvpsMapSequence->push_back(prNew);
							if (comment) {
								comment= false;
							} else {
								++size;
							}
						}
					} else if (isType(ARRAY)) {
						if (!(arrEnd && obj->isType(NUL))) {
							val.array->push_back(obj);
							++size;
						} else if (
							(obj->isType(NUL) || obj->isType(UNDEFINED)) &&
							obj->isQType(NONE) &&
							!obj->isEFlagSet((E_FLAGS)(FILE|CASTFILE))
						) {
							delete obj;
							obj= nullptr;
							if (!arrEnd || size) {
								val.array->push_back(NULL);
								++size;
							}
						}
						arrInd= to_string(size);
						// bool bLastObjIsMulLnStr= obj && obj->isType(STRING) &&
						// 	m_uFM.m_bIsMultiLineArray &&
						// 	(txj[i]=='\t' || txj[i]=='\n' || txj[i]=='\r');
						// while (txj[i]==' '|| txj[i]=='\t') ++i;
						// bool bEndOfMulLnStrArr=
						// 	(m_uFM.m_bIsMultiLineArray &&
						// 	 (txj[i]=='\n' || txj[i]=='\r'));
						// if (!bLastObjIsMulLnStr && !bEndOfMulLnStrArr) {
						// 	while (txj[i]!=',' && txj[i]!=']' && i<j) {
						// 		++i;
						// 	}
						// }
						// if (bEndOfMulLnStrArr) {
						// 	ReadMultiLinesInContainers(txj, i, ffpo);
						// 	m_uFM.m_bIsMultiLineArray= false;
						// }
					} else if (isType(UNDEFINED)){
						if (&objId!=&arrInd) {
							setType(ORDERED_OBJ);
							ffpo.m_pvpsMapSequence= fmMapSequence.m_pvpsMapSequence=
								new vector<ffmap::iterator>();
							insertFeaturedMember(fmMapSequence, FM_MAP_SEQUENCE);
						} else {
							setType(ARRAY);
							val.array= new vector<Txj_*>();
						}
						goto memobtained;
					} else {
						flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
						unlock();return;
					}
				} else if (txji=='{') {
					if (isType(OBJ)) {
					  ObjIns:
						if (&objId==&arrInd) {
							flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
							unlock();return;
						}
						prNew= val.pairs->find(objId);
						prNew->second= obj;
						if (isType(ORDERED_OBJ)) {
							goto SeqIns;
						}
						++size;
					} else if (isType(SET_TYPE)) {
						if (!obj->isType(UNDEFINED) && !obj->isType(NUL)) {
							if (&objId!=&arrInd) {
								flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
								unlock();return;
							}
							pair<ffset::iterator,bool> ret= val.setPtr->insert(obj);
							if (ret.second) {
								++size;
							} else {
								delete obj;
							}
						} else {
							delete obj;
							if (!size && arrEnd) {
								delete val.setPtr;
								val.setPtr= nullptr;
								setType(UNDEFINED);
								goto backyard;
							}
						}
					} if (isType(UNDEFINED)) {
						if (&objId!=&arrInd || (arrEnd && obj->isType(NUL))) {
							setType(OBJ);
							if (val.pairs==nullptr) {
								val.pairs= new ffmap();
								if (arrEnd) {
									delete obj;
									goto backyard;
								}
							}
						} else {
							setType(SET_TYPE);
							val.setPtr= new set<Txj_*, FFPtrCmp>();
						}
						goto memobtained;
					}
				} else {
					flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
					unlock();return;
				}
				if (arrEnd) {
					goto backyard;
				}
			}
			break;
		case ':': 
			if (txj[i+1]=='/' && txj[i+2]=='/') {
				if (txj[i-1]=='e' && txj[i-2]=='l' && txj[i-3]=='i' &&
					 txj[i-4]=='f' && (i<=5 || txj[i-5]<'a' || txj[i-5]>'z')
				) {
					i+= 3;
					string path;
					string objCaster;
					bool objCastNail= false;
					char txjii= txj[i];
					while (i<j && !isTerminatingChar(txjii)) {
						if (!isWhiteSpace(txjii)) {
							if (objCastNail) {
								if (txjii=='.') {
									objCastNail= false;
								} else
									objCaster+= txjii;
							} else if (!objCaster.length() && txjii=='.') {
								objCastNail= true;
							}
							path+= txjii;
						}
						++i; txjii= txj[i];
					}
					if (objCastNail) objCaster.erase();
					if (path.length()>0) {
						if (path[0]!='/') {
							TxjPObj* lpobj= pObj;
							while (lpobj) {
								if (lpobj->value->isEFlagSet(
										 (E_FLAGS)(FILE|CASTFILE))) {
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
						ifstream ifs(path.c_str(), ios::in|ios::ate);
						setEFlag(FILE);
						FeaturedMember fm;
						fm.m_sFileName= new char[path.length()+1];
						strcpy(fm.m_sFileName, path.c_str());
						insertFeaturedMember(fm, FM_FILE);
						if (ifs.is_open()) {
							string txjStr;
							ifs.seekg(0, ios::end);
							uint8_t t= UNDEFINED;
							if (objCaster.length()>0) {
								setEFlag(CASTFILE);
								txjStr.reserve((int)ifs.tellg()+3);
								t= STR_OBJ[objCaster];
								switch (t) {
								case OBJ:
								case SET_TYPE: txjStr+= "{"; break;
								case ORDERED_OBJ:
								case ARRAY: txjStr+= "["; break;
								case STRING:
									txjStr+= "\"\n";
								}
							} else {
								txjStr.reserve(ifs.tellg());
							}
							ifs.seekg(0, ios::beg);
							txjStr.append((istreambuf_iterator<char>(ifs)),
											  istreambuf_iterator<char>());
							ifs.close();
							if (objCaster.length()>0) {
							switch (t) {
							case OBJ:
							case SET_TYPE: txjStr+= "}"; break;
							case ORDERED_OBJ:
							case ARRAY: txjStr+= "]"; break;
							case STRING:
								txjStr+= "\"";
							}}
							unlock();
							init(txjStr, NULL, 0, pObj);
							lock();
						} else {
							flWrn(TXJ_MAIN, "Couldn't open %s", path.c_str());
							ifs.close();
						}
					}
				} else {
					while (i<j&&!isTerminatingChar(txj[i])) {
						++i;
					}
				}
			goto backyard;
			}
			buf= txj.substr(objIdNail, i-objIdNail);
		foundKey: {
				if (!pObj || !pObj->value) {
					flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
					unlock();return;
				}
				ffmap*& lp= pObj->value->val.pairs;
				if (!lp) {
					lp= new ffmap();
				} else if (!(pObj->value->isType(OBJ) ||
								 pObj->value->isType(ORDERED_OBJ))) {
					flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
					unlock();return;
				}
				trimWhites(buf);
				trimQuotes(buf);
				pObj->name= &lp->insert(
					pair<string, Txj_*>(buf, nullptr)).first->first;
				objIdNail= i+1;
			};
			break;
		case '"': {
			++i; val.str= NULL;
			int nind= getIndent(txj.c_str(), &i, indent);
			bool bMultiLineTxt= false;
			//FeaturedMember fmWidth;
			char txjii;
			buf.clear();
			while (i<j) {
				txjii= txj[i];
				switch (txjii) {
				case '\\': ++i;
					switch (txj[i]) {
					case 'n': buf+= '\n'; break;
					case 'r': buf+= '\r'; break;
					case 't': buf+= '\t'; break;
					case '\\': buf+= '\\'; break;
					default: buf+= txj[i]; break;}
					++i; break;
				case '\r': if (!txj[i+1]=='\n') break;
				case '\n': {
					bool lMulLn= false;
					if (txj[i-1]=='"') {bMultiLineTxt= true; lMulLn= true;}
					if (txjii=='\r')
						++i;
					int ind= ++i;
					while (bMultiLineTxt && (ind-i)<=nind) {
						if (txj[ind]=='\t')++ind;
						else if (nind==ind-1) {
							if (txj[ind]!='"') {
								flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
								unlock();return;
							}
							break;
						}
						else --nind;
					}
					if ((ind-i)==nind) {
						i+= nind;
					} else if (txj[ind]=='"' && ((ind-i)==(nind-1))) {
						i= ind+1;
						break;
					} else {
						i= ind;
					}
					if (!lMulLn && txj[i]!='"') buf+= '\n'; break;}
				case '"': {
					int l= i;
					while (l && l<j) {
						++l;
						switch (txj[l]) {
						case ',':
						case ']':
						case '}': l= 0; break;
						case ':': i= l; goto foundKey;}
					}
					++i; goto assignStr;}
				default: buf+= txj[i]; ++i; break;}
			}
		  assignStr: 
			setType(STRING);
			size= buf.length();
			nind= 1;
			while (size && nind && size-nind>=0) {
				txjii= buf[size-nind];
				switch (txjii) {
				case '\n':
				case '\t': buf.pop_back(); ++nind; break;
				default: nind=0; break;}
			}
			size= buf.length();
			val.str= new char[size+1];
			memcpy(val.str, buf.c_str(), size);
			val.str[size]= '\0';
			goto backyard;
		}
		case '<': {
			++i;
			int xmlNail= i;
			string xmlTag;
			int length= -1;
			bool tagset= false;
			while (txj[i]!='>' && i<j) {
				if (txj[i]==' ') {
					tagset= true;
					if (txj[i+1]=='l' && txj[i+2]=='e' && txj[i+3]=='n' &&
						 txj[i+4]=='g' && txj[i+5]=='t' && txj[i+6]=='h') {
						i+= 7;
						while (txj[i]!='=' && i<j) {
							++i;
						}
						++i;
						while (txj[i]!='"' && i<j) {
							++i;
						}
						++i;
						string lengthstr;
						while (txj[i]!='"' && i<j) {
							lengthstr+= txj[i];
							++i;
						}
						length= atoi(lengthstr.c_str());
					}
				} else if (!tagset) {
					xmlTag+= txj[i];
				}
				++i;
			}
			val.str= NULL;
			setType(XML);
			++i;
			xmlNail= i;
			if (length>-1 && length<(j-i)) {
				i+= length;
			}
			while (i<j) {
				if (txj[i]=='<' && txj[i+1]=='/') {
					if (xmlTag.compare(txj.substr(i+2, xmlTag.length()))==0 &&
						 txj[i+2+xmlTag.length()]=='>') {
						size= i-xmlNail;
						val.str= new char[size];
						memcpy(val.str, txj.c_str()+xmlNail, size);
						i+= 3+xmlTag.length();
						break;
					}
				}
				++i;
			}
			if (val.str==NULL)setType(NUL);
			goto backyard;
		}
		case '(': {
			++i;
			int typeNail= i;
			while (txj[i]!=')' && i<j) {
				++i;
			}
			if (i==j) {
				flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
				unlock();return;
			}
			size= atoi(txj.c_str()+typeNail);
			++i;
			if (size<0 || size>j-i) {
				flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
				unlock();return;
			}
			val.vptr= (uint8_t*)malloc(size*sizeof(uint8_t));
			memcpy(val.vptr, txj.c_str()+i, size);
			setType(Txj_::BINARY);
			i+= size;
			goto backyard;
		}
		case 't': {
			if (txj[i+1]=='r' && txj[i+2]=='u' && txj[i+3]=='e') {
				setType(BOOL);
				val.boolean= true;
				i+= 4;
				goto backyard;
			}
			break;
		}
		case 'f': {
			if (txj[i+1]=='a' && txj[i+2]=='l' && txj[i+3]=='s' &&
				 txj[i+4]=='e') {
				setType(BOOL);
				val.boolean= false;
				i+= 5;
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
		case '+': if (i>0 && !isInitializingChar(txj[i-1])) break; {
			int numNail= i;
			++i;
			int precision= 0;
			while ((txj[i]>='0' && txj[i]<='9') ||
					 (txj[i]=='.' && i+1<j && (txj[i+1]>='0' && txj[i+1]<='9'))) {
				if (txj[i]=='.') {
					precision= i-numNail+1;
				}
				++i;
			}
			if (i-numNail==1 && !(txj[numNail]>='0' && txj[numNail]<='9')) {
				break;
			}
			if (!isWhiteSpace(txj[i]) && !isTerminatingChar(txj[i])) {
				break;
			}
			size= i-numNail;
			if (!precision)
				precision= size;
			string num= txj.substr(numNail, i-numNail);
			if (num.length()==20) {
				size= sizeof(FerryTimeStamp);
				val.m_pFerryTimeStamp= (FerryTimeStamp*)malloc(size);
				*val.m_pFerryTimeStamp= FerryTimeStamp(num);
				setType(BINARY);
				goto backyard;
			}
			size_t s= 0;
			val.number= stod(num, &s);
			FeaturedMember cFM;
			cFM.precision= size-precision;
			if (cFM.precision) {
				setEFlag(PRECISION);
				insertFeaturedMember(cFM, FM_PRECISION);
			}
			setType(NUMBER);
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
			if (txj[i+1]=='e' && txj[i+2]=='l' && txj[i+3]=='e' &&
				 txj[i+4]=='t' && txj[i+5]=='e') {
				setQType(DEL);
				i+= 6;
				goto backyard;
			}
			break;
		}
		case '!': {
			if (!pObj || !pObj->value) {
				flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
				unlock();return;
			}
			setType(NUL);
			pObj->value->setQType(NQUERY);
			goto backyard;
		}
		case 'n': {
			if (txj[i+1]=='u' && txj[i+2]=='l' && txj[i+3]=='l') {
				setType(NUL);
				i+= 4;
				goto backyard;
			}
			break;
		}
		case '}':
		case ']':
		case ',': {
			// NULL Objects or links caught here eg. "[]", ",,", ",]", "name:,}"
			string subffj(txj.c_str()+objIdNail, i-objIdNail);
			trimWhites(subffj);
			if (subffj.length()>0) {
				vector<string>* prop= new vector<string>();
				explode(".", subffj, *prop);
				Txj_* obj= returnNameIfDeclared(*prop, pObj);
				if (!obj) {
					if (!pObj) {
						delete prop;
						setType(UNDEFINED);
						goto backyard;
					}
					int pL= 0;
					TxjPObj* lfpo= pObj;
					while (pL<prop->size() && !(*prop)[pL].size()) {
						lfpo= lfpo->pObj;
						if (!lfpo) {
							delete prop;
							setType(UNDEFINED);
							goto backyard;
						}
						++pL;
					}
					SymlinkTrail sT;
					sT.l= prop;
					sT.ln= this;
					lfpo->symTrVec.push_back(sT);
				}
				setType(LINK);
				val.fptr= obj;
				FeaturedMember cFM;
				cFM.link= prop;
				insertFeaturedMember(cFM, FM_LINK);
			} else {
				setType(NUL);
				size= 0;
			}
			goto backyard;
		}
		}
		++i;
	}
  backyard:
	if (!isType(UNDEFINED) &&
		 !(isType(STRING) && fmMulLnBuf.m_psMultiLnBuffer)) {
		while ((txj[i]==' ' || txj[i]=='\t') && i<j) {
			++i;
		}
		if (txj[i]=='|') {
			++i;
			ffpo.name= nullptr;
			ffpo.pObj= pObj;
			ffpo.value= this;
			ffpo.m_pvpsMapSequence= nullptr;
			Txj_* obj= new Txj_(txj, &i, indent, &ffpo);
			if (inherit(*obj, &ffpo)) {

			} else {
				delete obj;
			}
		}
	} /*else if (isType(UNDEFINED)) {
		 int k=*ci;
		 while ((txj[k]==' ' || txj[k]=='\t') && k<j) ++k;
		 ffpo.value= this;
		 ffpo.pObj= pObj;
		 TxjPObj* pPObj= pObj;
		 Txj_* link=nullptr;
		 Iterator it;
		 string key;
		 bool done=false;
		 while(true) {
		 switch(txj[k]) {
		 case '.':
		 if (txj[k+1]=='.') {
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
		 if(link->isType(OBJ)||link->isType(ARRAY))
		 link=&(*link)[key];
		 }else{
		 flWrn(TXJ_MAIN, "%s not found in %s at %d",
		 key.c_str(),GetLinkString(pObj).m_sLink,k);
		 break;
		 }
		 it= pPObj->value->find(key);
		 if(it!=pPObj->value->end()) {
		 link=&(*pPObj->value)[key];
		 }
		 key="";
		 break;
		 default:
		 key+=txj[k];
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
	for (int l= ffpo.symTrVec.size()-1; l>=0; --l) {
		Txj_* tln= this;
		for (int m= 0; m<ffpo.symTrVec[l].l->size(); ++m) {
			if (!(*ffpo.symTrVec[l].l)[m].size()) {
				continue;
			}
			Txj_::Iterator it= tln->find((*ffpo.symTrVec[l].l)[m]);
			if (it==tln->end()) {
				tln= nullptr;
				break;
			} else {
				tln= &*it;
			}
		}
		if (tln) {
			ffpo.symTrVec[l].ln->val.fptr= tln;
		} else if (pObj) {
			pObj->symTrVec.push_back(ffpo.symTrVec[l]);
		} else {
			ffpo.symTrVec[l].ln->freeObj();
		}
	}
	unlock();
}

void Txj_::ReadMultiLinesInContainers(const string& ffjson, int& i,
													 TxjPObj & pObj) {
	if (ffjson[i]=='\n' || ffjson[i]=='\r') {
		if (ffjson[i]=='\r')++i;
		int iI= 0;
		int iTxjSize= ffjson.length();
		int iArrSize= pObj.value->val.array->size();
		while (iI<iArrSize) {
			while (ffjson[i]==' ' || ffjson[i]=='\t')++i;
			if (ffjson[i]=='\n' || ffjson[i]=='\r') {
				if (ffjson[i]=='\r')++i;
				++i;
				iI= 0;
				continue;
			}
			FeaturedMember fmMulLnBuf= (*pObj.value->val.array)[iI]->
				getFeaturedMember(FM_MULTI_LN);
			while (!((*pObj.value->val.array)[iI]->isType(STRING) &&
						fmMulLnBuf.m_psMultiLnBuffer!=NULL)) {
				++iI;
				if (iI>=iArrSize)break;
				fmMulLnBuf= (*pObj.value->val.array)[iI]->
					getFeaturedMember(FM_MULTI_LN);
			}
			if (iI==iArrSize)break;
			string& sTemp= *fmMulLnBuf.m_psMultiLnBuffer;
			bool bBreak= false;
			while (!bBreak && i<iTxjSize) {
				switch (ffjson[i]) {
					case '\\':
						++i;
						switch (ffjson[i]) {
							case 'n':
								sTemp+= '\n';
								break;
							case 'r':
								sTemp+= '\r';
								break;
							case 't':
								sTemp+= '\t';
								break;
							case '\\':
								sTemp+= '\\';
								break;
							default:
								sTemp+= ffjson[i];
								break;
						}
						++i;
					case '\r':
						++i;
					case '\n':
						iI= 0;
						bBreak= true;
						sTemp+= '\n';
						++i;
						break;
					case '\t':
						++iI;
						bBreak= true;
						sTemp+= '\n';
						++i;
						break;
					case '"':
						*(*pObj.value->val.array)[iI]= sTemp;
						//(*pObj.value->val.array)[iI]->
						//deleteFeaturedMember(FM_MULTI_LN);
						delete &sTemp;
						if (iI<iArrSize-1) while (ffjson[i]!=',')++i;
						++i;
						bBreak= true;
						++iI;
						break;
					default:
						sTemp+= ffjson[i];
						++i;
						break;
				}
			}
		}
	}
}

void Txj_::setFMCount(uint32_t iFMCount) {
	iFMCount <<= 28;
	flags &= 0x0FFFFFFF;
	flags |= iFMCount;

}

// fifo: first in first out, last fmh holds 2 fms. should be inserted in
// order
void Txj_::insertFeaturedMember (FeaturedMember& fms, FeaturedMemType fMT) {
	FeaturedMember* pFMS= &m_uFM;
	uint32_t iFMCount= flags>>28;
	uint32_t iFMTraversed= 0;
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fMT==FM_FILE) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
	}
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fMT==FM_MULTI_LN) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (isType(STRING)) {
		if (fMT==FM_WIDTH) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (isType(ORDERED_OBJ)) {
		if (fMT==FM_MAP_SEQUENCE) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (fMT==FM_TABHEAD) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (this->isType(LINK)) {
		if (fMT==FM_LINK) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXTENDED) &&
		 (isType(OBJ) || isType(ORDERED_OBJ) || isType(ARRAY))) {
		if (fMT==FM_PARENT) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
		if (fMT==FM_TABHEAD) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJ) || isType(ARRAY))) {
		if (fMT==FM_CHILDREN) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
	if (isQType(UPDATE)) {
		if (fMT==FM_UPDATE_TIMESTAMP) {
			if (!pFMS->width) {
				*pFMS= fms;
			} else {
				FeaturedMemHook* pNewFMH= new FeaturedMemHook();
				pNewFMH->m_uFM= *pFMS;
				pNewFMH->m_pFMH= fms;
				pFMS->m_pFMH= pNewFMH;
			}
			iFMCount++;
			setFMCount(iFMCount);
			return;
		} else if (iFMCount>1) {
			pFMS= &pFMS->m_pFMH->m_pFMH;
		}
		++iFMTraversed;
	}
}

Txj_::FeaturedMember Txj_::getFeaturedMember (FeaturedMemType fMT) const {
	const FeaturedMember* pFMS= &m_uFM;
	uint32_t iFMCount= flags>> 28;
	uint32_t iFMTraversed= 0;
	FeaturedMember decoyFM;
	decoyFM.precision= 0;
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fMT==FM_FILE) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fMT==FM_MULTI_LN) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(STRING)) {
		if (fMT==FM_WIDTH) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(ORDERED_OBJ)) {
		if (fMT==FM_MAP_SEQUENCE) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (fMT==FM_TABHEAD) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isType(LINK)) {
		if (fMT==FM_LINK) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isEFlagSet(EXTENDED) && (isType(OBJ) || isType(ARRAY))) {
		if (fMT==FM_PARENT && !isEFlagSet(EXT_VIA_PARENT)) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
		if (fMT==FM_TABHEAD) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJ) || isType(ARRAY))) {
		if (fMT==FM_CHILDREN) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	if (isQType(UPDATE)) {
		if (fMT==FM_UPDATE_TIMESTAMP) {
			if (iFMCount-iFMTraversed==1) {
				return *pFMS;
			} else {
				return pFMS->m_pFMH->m_uFM;
			}
		} else {
			if (iFMCount-iFMTraversed==1) {
				return decoyFM;
			} else {
				pFMS= &pFMS->m_pFMH->m_pFMH;
			}
		}
		++iFMTraversed;
	}
	return decoyFM;
}

void DeleteChildLinks(vector<Txj_*>* childLinks) {
	for (int i= 0; i<childLinks->size(); ++i) {
		delete (*childLinks)[i];
	}
}

void Txj_::destroyAllFeaturedMembers (bool bExemptQueries) {
	uint32_t iFMCount= flags >> 28;
	FeaturedMemHook* pFMH;
	if (!iFMCount)
		return;
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (iFMCount==1) {
			delete[] m_uFM.m_sFileName;
		} else {
			pFMH= m_uFM.m_pFMH;
			delete[] pFMH->m_uFM.m_sFileName;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
		clearEFlag(FILE);
	}
	if (isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (iFMCount==1) {
			//Deletion of the multilinebuf is responsibility of me
			//delete fmHolder.m_psMultiLnBuffer;
		} else {
			//Deletion of the multilinebuf is responsibility of me
			//delete fmHolder.m_pFMH->m_uFM.m_psMultiLnBuffer;
			pFMH= m_uFM.m_pFMH;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
		clearEFlag(STRING_INIT);
	}
	if (isType(STRING)) {
		if (iFMCount==1) {
			m_uFM.width= 0;
		} else {
			pFMH= m_uFM.m_pFMH;
			pFMH->m_uFM.width= 0;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
		setType(UNDEFINED);
	}
	if (isType(ORDERED_OBJ)) {
		if (iFMCount==1) {
			delete m_uFM.m_pvpsMapSequence;
		} else {
			pFMH= m_uFM.m_pFMH;
			delete pFMH->m_uFM.m_pvpsMapSequence;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	if (isEFlagSet(EXT_VIA_PARENT) && !isType(STRING)) {
		if (iFMCount==1) {
			//delete m_uFM.tabHead; //extended obj deletes it
		} else {
			pFMH= m_uFM.m_pFMH;
			//delete pFMH->m_uFM.tabHead;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	if (isType(LINK)) {
		if (iFMCount==1) {
			delete m_uFM.link;
		} else {
			pFMH= m_uFM.m_pFMH;
			delete pFMH->m_uFM.link;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	if (isEFlagSet(EXTENDED) && (isType(ORDERED_OBJ) || isType(ARRAY))) {
		if (iFMCount==1) {
			delete m_uFM.m_pParent;
		} else {
			pFMH= m_uFM.m_pFMH;
			delete pFMH->m_uFM.m_pParent;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
		if (iFMCount==1) {
			delete m_uFM.tabHead;
		} else {
			pFMH= m_uFM.m_pFMH;
			delete pFMH->m_uFM.tabHead;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	if (isEFlagSet(HAS_CHILDREN) && (isType(OBJ) || isType(ARRAY))) {
		if (iFMCount==1) {
			DeleteChildLinks(m_uFM.m_pvChildren);
			delete m_uFM.m_pvChildren;
		} else {
			pFMH= m_uFM.m_pFMH;
			DeleteChildLinks(pFMH->m_uFM.m_pvChildren);
			delete pFMH->m_uFM.m_pvChildren;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	if (isQType(UPDATE) && !bExemptQueries) {
		if (iFMCount==1) {
			m_uFM.m_pTimeStamp= NULL;
		} else {
			pFMH= m_uFM.m_pFMH;
			pFMH->m_uFM.m_pTimeStamp= NULL;
			m_uFM= pFMH->m_pFMH;
			delete pFMH;
		}
		iFMCount--;
	}
	setFMCount(iFMCount);
}

void Txj_::nullFeaturedMember (FeaturedMemType fmt) {
	FeaturedMember* pFMS= &m_uFM;
	uint32_t iFMCount= flags >> 28;
	uint32_t iFMTraversed= 0;
	if (isType(ORDERED_OBJ) && fmt==FM_MAP_SEQUENCE) {
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
void Txj_::deleteFeaturedMember (FeaturedMemType fmt) {
	FeaturedMember* pFMS= &m_uFM;
	uint32_t iFMCount= flags >> 28;
	uint32_t iFMTraversed= 0;
	FeaturedMember* pfmPre= NULL;
	if (this->isType(STRING) && isEFlagSet(STRING_INIT)) {
		if (fmt==FM_MULTI_LN) {
			if (iFMCount==1) {
				//delete pFMS->m_psMultiLnBuffer;
			} else if (iFMCount-iFMTraversed==1) {
				//delete pFMS->pFMH.m_pFMH.m_psMultiLnBuffer;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.m_psMultiLnBuffer;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
			clearEFlag(STRING_INIT);
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (this->isType(STRING)) {
		if (fmt==FM_WIDTH) {
			// can't be deleted as every object should contain MapSequence
			return;
			if (iFMCount-iFMTraversed==1) {
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (isType(ORDERED_OBJ) && fmt==FM_MAP_SEQUENCE) {
		if (iFMCount==1) {
			delete m_uFM.m_pvpsMapSequence;
			m_uFM.m_pvpsMapSequence= NULL;
		} else if (iFMCount-iFMTraversed==1) {
			//delete pFMS->pFMH.m_pFMH.m_psMultiLnBuffer;
			if (pfmPre) {
				*pfmPre= pfmPre->m_pFMH->m_uFM;
			}
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
		if (fmt==FM_TABHEAD) {
			return; // Inheritance can't be removed. Will think of it later.
			if (iFMCount-iFMTraversed==1) {
				if (this->isEFlagSet(EXTENDED) && getType()!=NUMBER) {
					delete pFMS->tabHead;
				}
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				if (this->isEFlagSet(EXTENDED) && getType()!=NUMBER) {
					delete pFMS->m_pFMH->m_uFM.tabHead;
				}
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (this->isType(LINK)) {
		if (fmt==FM_LINK) {
			return; //A link should have a Link
			if (iFMCount-iFMTraversed==1) {
				delete pFMS->link;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.tabHead;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (this->isEFlagSet(EXTENDED)) {
		if (fmt==FM_TABHEAD) {
			// Inheritance can't be removed. will think of it later.
			if (iFMCount-iFMTraversed==1) {
				delete pFMS->m_pParent;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.m_pParent;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
		if (fmt==FM_PARENT && !isEFlagSet(EXT_VIA_PARENT)) {
			// Inheritance can't be removed. will think of it later.
			if (iFMCount-iFMTraversed==1) {
				delete pFMS->m_pParent;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete pFMS->m_pFMH->m_uFM.m_pParent;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (this->isEFlagSet(HAS_CHILDREN) && (isType(OBJ) || isType(ARRAY))) {
		if (fmt==FM_CHILDREN) {
			return; //Inheritance can't be remove. will think of it later
			if (iFMCount-iFMTraversed==1) {
				DeleteChildLinks(pFMS->m_pvChildren);
				delete pFMS->m_pvChildren;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				DeleteChildLinks(pFMS->m_pFMH->m_uFM.m_pvChildren);
				delete pFMS->m_pFMH->m_uFM.m_pvChildren;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		if (fmt==(FeaturedMemType)FILE) {
			if (iFMCount-iFMTraversed==1) {
				delete[] pFMS->m_sFileName;
				if (pfmPre) {
					*pfmPre= pfmPre->m_pFMH->m_uFM;
				}
			} else {
				delete[] pFMS->m_pFMH->m_uFM.m_sFileName;
				FeaturedMember* pTempFMS= &pFMS->m_pFMH->m_pFMH;
				delete pFMS->m_pFMH;
				*pFMS= *pTempFMS;
			}
		}
		pfmPre= pFMS;
		++iFMTraversed;
	}
}

Txj_* Txj_::returnNameIfDeclared (
	vector<string>& prop, Txj_::TxjPObj* fpo
) const {
	int j= 0;
	int lp=0;
	if (fpo==nullptr){
		fpo=new Txj_::TxjPObj();
		fpo->value=const_cast<Txj_*>(this);
		for (int i=0; i<prop.size();++i) {
			if (!prop[i].size()) {
				++lp;
			} else {
				break;
			}
		}
	}
	while (fpo!=nullptr) {
		Txj_* fp= fpo->value;
		j= lp;
		while (j<prop.size()) {
			if (!prop[lp].size()) {
				++lp;
				break;
			}
			if (fp->isType(OBJ) || fp->isType(ORDERED_OBJ)) {
				if (fp->val.pairs->find(prop[j])!=fp->val.pairs->end()) {
					fp= (*fp->val.pairs)[prop[j]];
				} else {
					break;
				}
			} else if (fp->isType(ARRAY)) {
				int index= -1;
				index= atoi(prop[j].c_str());
				if (index==0 && prop[j][0]!='0') {
					break;
				}
				if (index<fp->size) {
					fp= (*fp->val.array)[index];
				}
			} else if (fp->isType(LINK)) {
				fp= fp->val.fptr;
			} else {
				break;
			}
			++j;
			if (j==prop.size())return fp;
		}
		if (fpo->value==this) {
			delete fpo;
			return nullptr;
		}
		fpo= fpo->pObj;
	}
	return nullptr;
}

int Txj_::getIndent (const char* txj, int* ci, int indent) {
	int i= *ci;
	if (txj[i]=='\n') {
		++i;
	} else if (txj[i]=='\r' && txj[i+1]=='\n') {
		i+= 2;
	} else {
		return indent;
	}
	int j= i+indent+1;
	while (i<j) {
		if (txj[i]!='\t') {
			return indent;
		}
		++i;
	}
	return (indent+1);
}

Txj_::~Txj_ () {
	lock();
	freeObj();
	unlock();
	MtxMapMtx.lock_shared();
	map<const Txj_*, shared_mutex>::iterator it= MtxMap.find(this);
	MtxMapMtx.unlock_shared();
	if (it!=MtxMap.end()) {
		it->second.lock();
		MtxMapMtx.lock();
		MtxMap.erase(it);
		MtxMapMtx.unlock();
	}
}

void Txj_::freeObj (bool bAssignment) {
	switch (getType()) {
	case ORDERED_OBJ:
	case OBJ: {
		ffmap::iterator i;
		i= val.pairs->begin();
		while (i!=val.pairs->end()) {
			delete i->second;
			++i;
		}
		delete val.pairs;
		break;}
	case ARRAY: {
		int i= val.array->size()-1;
		while (i>=0) {
			delete (*val.array)[i];
			--i;
		}
		delete val.array;
		break;}
	case SET_TYPE: {
		for (Txj_* fp : *val.setPtr) {
			if (fp)
				delete fp;
		}
		delete val.setPtr;
		break;}
	case STRING: {}
	case XML: {
		delete[] val.str;
		break;}
	case LINK: break;
	case DLINK: break;
	case BINARY: free(val.vptr); break;
	case TIME: delete val.m_pFerryTimeStamp;
	default: break;}
	
	if (m_uFM.m_pFMH!=nullptr) {
		destroyAllFeaturedMembers(bAssignment);
		m_uFM.m_pFMH=nullptr;
	}
	val.fptr= 0;
	size= 0;
	flags&= bAssignment?0xF000FF00:0;
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
void Txj_::trimWhites (string& s) {
	int i= 0;
	int j= s.length()-1;
	while (i<=j && isWhite(s[i])) {
		++i;
	}
	while (i<=j && isWhite(s[j])) {
		--j;
	}
	s= i <= j ? s.substr(i, ++j-i) : "";
}

void Txj_::trimQuotes (string& s) {
	int i= 0;
	int j= s.length()-1;
	if (j<=0) {
		return;
	}
	if (s[0]=='"') {
		++i;
	}
	if (s[j]=='"') {
		--j;
	}
	++j;

	s= s.substr(i, j-i);
}

Txj_::OBJ_TYPE Txj_::objectType (string ffjson) {
	if (ffjson[0]=='{' && ffjson[ffjson.length()-1]=='}') {
		return OBJ;
	} else if (ffjson[0]=='"' && ffjson[ffjson.length()-1]=='"') {
		return STRING;
	} else if (ffjson[0]=='[' && ffjson[ffjson.length()-1]==']') {
		return ARRAY;
	} else if (ffjson.compare("true")==0 || ffjson.compare("false")==0) {
		return BOOL;
	} else {
		return UNDEFINED;
	}
}

Txj_& Txj_::operator [] (void) {
	lock();
	if (isLink()) {
		unlock();
		return (*val.fptr)[];
	} else if (isType(UNDEFINED)) {
	  settype:
		setType(SET_TYPE);
		val.setPtr= new ffset();
	} else if (isType(OBJ) && size==0) {
		freeObj();
		goto settype;
	}
	Txj_* obj= new Txj_();
	if (isType(SET_TYPE)) {
		obj->setType(NEW_SET_MEMBER);
		obj->val.fptr= this;
	}
	unlock();
	return *obj;
}

Txj_& Txj_::operator [] (const string& prop) {
	return (*this)[prop.c_str()];
}
 
Txj_& Txj_::operator [] (const char* prop) {
	if (isLink()) {
		return (*val.fptr)[prop];
	}
	Txj_* obj;
	switch (getType()) {
	case UNDEFINED: {
		setType(OBJ);
		val.pairs= new ffmap();
		size= 0;}
	case OBJ:
	case ORDERED_OBJ: {
		string t(prop);
		lockShared();
		ffmap::iterator it= val.pairs->find(t);
		unlockShared();
		if (it!=val.pairs->end()) {
			if (it->second!=NULL) {
				Txj_* prf= it->second;
				while (prf->isLink()) {
					prf= prf->val.fptr;
				}
				return *prf;
			} else {
				obj= new Txj_();
				return *((*val.pairs)[t]= obj);
			}
		} else {
			obj= new Txj_();
			lock();
			++size;//should b increase only after getFeatruredMember
			pair<ffmap::iterator, bool> prNew= val.
				pairs->insert(pair<string, Txj_*>(string(prop), obj));
			FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
			if (fmMapSequence.m_pvpsMapSequence) {
				fmMapSequence.m_pvpsMapSequence->push_back(prNew.first);
			}
			unlock();
			return *obj;
		}}
	case ARRAY:
		if (isEFlagSet(EXT_VIA_PARENT) || isEFlagSet(EXTENDED)) {
			FeaturedMember cFM= getFeaturedMember(FM_TABHEAD);
			int i= (*cFM.tabHead)[prop];
			Txj_& ret= (*this)[i]; //no need to lock; [i] will lock
			return ret;
		} else {
			Txj_& ret= (*this)[atoi(prop)];
			return ret;
		}
	case SET_TYPE: {
		ffset::iterator it= val.setPtr->begin();
		while (it!=val.setPtr->end()) {
			Txj_* pt= *it;
			++it;
			if (pt->isType(Txj_::STRING)) {
				if (!strcmp(prop, pt->val.str)) {
					return *pt;
				}
			}
		}}
	}
	if (!obj) {
		obj= &nullTxj;
	}
	return *obj;
}

Txj_& Txj_::operator [] (const int index) {
  ssstart:
	lockShared();
	if (isType(ARRAY)) {
		if (val.array->size()>index) {
			if ((*val.array)[index]==NULL) {
				unlockShared();lock();
				(*val.array)[index]= new Txj_(NUL);
				unlock();lockShared();
			} else if ((*val.array)[index]->isLink()) {
				Txj_* prf= (*val.array)[index];
				while (prf->isLink()) {
					prf= prf->val.fptr;
				}
				unlockShared();
				return *prf;
			}
			Txj_& ret= *((*val.array)[index]);
			unlockShared();
			return ret;
		} else {
			unlockShared();lock();
			if (val.array->size()>index) {
				Txj_& ret= *((*val.array)[index]);
				unlock();
				return ret;
			}
			Txj_* f;
			for (int i=size; i<=index; ++i) {
				f= new Txj_();
				val.array->push_back(f);
				++size;
			}
			unlock();
			return *f;
		}
	} else if (isType(UNDEFINED)) {
		unlockShared();lock();
		setType(ARRAY);
		size= 0;
		val.array= new vector<Txj_*>();
		Txj_* f;
		for (int i= size; i<=index; ++i) {
			f= new Txj_();
			val.array->push_back(f);
			++size;
		}
		unlock();
		return *f;
	} else if (isLink()) {
		unlockShared();
		return (*val.fptr)[index];
	} else {
		unlockShared();
		return nullTxj;
	}
};

Txj_& Txj_::operator * () {
	Txj_* fp= this;
	while (fp->isLink()) {
		fp= fp->val.fptr;
	}
	return *fp;
}

Txj_* Txj_::operator -> () {
	return &**this;
}

/**
 * converts Txj_ object to json string
 * @param encode_to_base64 if true then the binary data is base64 encoded
 * @return json string of this Txj_ object
 */
string Txj_::stringify (
	bool json, bool bGetQueryStr, TxjPObj* pObj, uint lnLvl
) const {
	string ffs;
	stringify(ffs, json, bGetQueryStr, pObj, lnLvl);
	return ffs;
}
void Txj_::stringify (
	string& ffs, bool json, bool bGetQueryStr, TxjPObj* pObj, uint lnLvl
) const {
	if (bGetQueryStr) {
		if (isQType(QUERY)) {
			ffs+= "?";
			return;
		} else if (isQType(DEL)) {
			ffs+= "delete";
			return;
		} else if (isQType(QUERY_TYPE::SET) || isType(OBJ) || isType(ARRAY)) {
		} else {
			return;
		}
	}
	char opnBrc, clsBrc;
	switch (getType()) {
	case STRING: {
		ffs.reserve(2 * size+2);
		ffs+= '"';
		int i= 0;
		while (i<size) {
			switch (val.str[i]) {
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
				ffs+= val.str[i];
				break;
			}
			++i;
		}
		ffs+= '"';
		return;
	}
	case NUMBER: {
		if (isEFlagSet(PRECISION)) {
			int precision= getFeaturedMember(FM_PRECISION).precision;
			string num(toPreciseStr(val.number, precision));
			ffs+= num;
			return;
		} else {
			string num(toPreciseStr(val.number, 0));
			ffs+= num;
			return;
		}
	}
	case XML: {
		if (isEFlagSet(B64ENCODE)) {
			int output_length= 0;
			char * b64_char= base64_encode((const unsigned char*) val.str,
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
			ffs.append(val.str, size);
			ffs+= "</xml>";
			return;
		}
	}
	case BOOL: {
		if (val.boolean) {
			ffs+= "true";
			return;
		} else {
			ffs+= "false";
			return;
		}
	}
	case ORDERED_OBJ: opnBrc= '['; clsBrc= ']'; goto setBrace;
	case OBJ: {
		opnBrc= '{'; clsBrc= '}';
	  setBrace:
		if (size==0) {
			ffs+= string(1, opnBrc)+clsBrc;
			return;
		}
		ffmap& objmap= *(val.pairs);
		ffs+= opnBrc;
		ffmap::iterator i;
		FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
		vector<ffmap::iterator>* itVecPtr= fmMapSequence.m_pvpsMapSequence;
		int iMapSeqIndexer= 0;
		if (itVecPtr) iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
		else i= objmap.begin();			
		TxjPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<Txj_*> (this);
		while (i!= objmap.end()) {
			uint32_t t= i->second ? i->second->getType() : NUL;
			if (t==UNDEFINED || i->first[0]=='#')
				goto iterPrint;
			if (t!= NUL) {
				if (isEFlagSet(B64ENCODE))i->second->setEFlag(B64ENCODE);
				if ((isEFlagSet(B64ENCODE_CHILDREN))&&
					 !isEFlagSet(B64ENCODE_STOP))
					i->second->setEFlag(B64ENCODE_CHILDREN);
			}
			if (json) {ffs+= '"';};
			ffs+= i->first;
			if (json) {ffs+= '"';};
			ffs+= ':';
			lfpo.name= &i->first;
			if (t!= NUL) {
				i->second->stringify(ffs, json, false, &lfpo, lnLvl);
			} else if (json) {
				ffs+= "null";
			}
			ffs.append(",");
		  iterPrint:
			iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
		}
		ffs.back()= clsBrc;
		break;
	}
	case SET_TYPE: {
		ffset& objset= *(val.setPtr);
		TxjPrettyPrintPObj lfpo;
		lfpo.pObj= pObj;
		ffs+= "{";
		for (Txj_* fp : objset) {
			fp->stringify(ffs, json, false, &lfpo, lnLvl);
			ffs+= ',';
		}
		ffs.back()= '}';
		break;
	}
	case ARRAY: {
		vector<Txj_*>& objarr= *(val.array);
		ffs+= "[";
		int i= 0;
		TxjPrettyPrintPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<Txj_*> (this);
		while (i<objarr.size()) {
			uint32_t t= objarr[i]? objarr[i]->getType(): NUL;
			if (t==NUL || t==UNDEFINED) {
				if (json) {
					ffs.append("null");
				}
			} else {
				if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
				if ((isEFlagSet(B64ENCODE_CHILDREN))&&
					 !isEFlagSet(B64ENCODE_STOP))
					objarr[i]->setEFlag(B64ENCODE_CHILDREN);
				objarr[i]->stringify(ffs, json, false, &lfpo, lnLvl);
			}
			if (++i!=objarr.size()) {
				if (objarr[i]) {
					ffs.append(",");
				}
			}
		}
		ffs+= ']';
		break;
	}
	case LINK: {
		vector<string>* vtProp= getFeaturedMember(FM_LINK).link;
		if ((lnLvl<=0 || returnNameIfDeclared(*vtProp, pObj)!=NULL) &&
//if ((returnNameIfDeclared(*vtProp, pObj)!=NULL) &&
			 !isEFlagSet(LONG_LAST_LN)
		) {
			string ln= implode(".", *vtProp);
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
		ffs+= "("+str+")";
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
			ffs+= json? "null" : "";
		}
	}}
	if (isEFlagSet(EXTENDED) && !isType(STRING)) {
		Txj_* pParent= getFeaturedMember(FM_PARENT).m_pParent;
		ffs+= '|';
		pParent->stringify(ffs, false, false, pObj,lnLvl);
	}
	return;
}

string Txj_::prettyString (
	bool json, bool printComments, int indent,
	TxjPrettyPrintPObj* pObj, bool printFilePath, bool save
) const {
	string ps;
	prettyString(ps, json, printComments, indent, pObj, printFilePath, save);
	return ps;
}
void Txj_::prettyString (
	string& ps, bool json, bool printComments, int indent,
	TxjPrettyPrintPObj* pObj, bool printFilePath, bool save
) const {
	char opnBrc, clsBrc;
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE)) && pObj && printFilePath) {
		ccp filename= getFeaturedMember(FM_FILE).m_sFileName;
		if (save)
			this->save(json, printComments, 0, pObj, false);
		int ri= strlen(filename)-1;
		while (ri>0 && (filename[ri]!='.' || filename[ri+1]!='/')) {
			--ri;
		}
		if (ri>0)
			ri+= 2;
		ps+= string("file://")+(filename+ri);
		return;
	}
	if (!printFilePath && save) {
		printFilePath= true;
	}
	if (save && isEFlagSet(CASTFILE)) {
		indent= -1;
	}
	switch (getType()) {
	case STRING: {
		ps+= '\"';
		char* pcNewLnCharPos= strchr(val.str, '\n');
		bool hasNewLine= (pcNewLnCharPos!=NULL);
		if (pObj && pObj->m_bGiveFirstLine) {
			if (hasNewLine) {
				ps.append(val.str, 0, pcNewLnCharPos-val.str);
				pObj->m_bGiveFirstLine= false;
			} else {
				ps.append(val.str);
				ps+= '"';
			}
		} else {
			if (hasNewLine) {
				ps+= '\n';
				ps.append(indent+1, '\t');
			}
			int stringNail= 0;
			int i= 0;
			if (hasNewLine) {
				for (i= 0; i<size; ++i) {
					if (val.str[i]=='\n') {
						ps.append(val.str, stringNail, i+1-stringNail);
						ps.append(indent+1, '\t');
						stringNail= i+1;
					}
				}
			} else {
				i= size;
			}
			int ii= stringNail;
			while (ii<size) {
				char ch= val.str[ii];
				switch (ch) {
				case '"':
					if (hasNewLine) {
						break;
					}
				case '\\':
					ps+= '\\';
					break;
				}
				ps+= ch;
				++ii;
			}
			if (hasNewLine) {
				ps+= '\n';
				ps.append(indent>0?indent:0, '\t');
			}
			ps+= "\"";
		}
		break;
	}
	case NUMBER: {
		if (isEFlagSet(PRECISION)) {
			int precision= getFeaturedMember(FM_PRECISION).precision;
			string num(toPreciseStr(val.number, precision));
			ps+= num;
			return;
		} else {
			ps+= toPreciseStr(val.number, 0);
			return;
		}
		break;
	}
	case XML: {
		if (isEFlagSet(B64ENCODE)) {
			int output_length= 0;
			char * b64_char= base64_encode(
				(const unsigned char*)val.str, size, (size_t*)&output_length);
			string b64_str(b64_char, output_length);
			free(b64_char);
			ps+= "\""+b64_str+"\"";
			return;
		} else {
			ps+= "<xml length= \""+to_string(size)+"\" >" +
				string(val.str, size)+"</xml>";
			return;
		}
		break;
	}
	case BOOL: if (val.boolean) {
			ps+= "true";
			return;
		} else {
			ps+= "false";
			return;
		}
		break;
	case ORDERED_OBJ: opnBrc= json?'{':'['; clsBrc= json?'}':']';
		goto setPPBrace;
	case OBJ: {
		opnBrc= '{'; clsBrc= '}';
	  setPPBrace:
		if (size==0) {
			ps+= (save && isEFlagSet(CASTFILE))? "":string(1, opnBrc)+clsBrc;
			return;
		}
		ffmap& objmap= *val.pairs;
		ffmap::iterator i;
		map<string, vector<int> > msviClWidths;
		FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
		int iMapSeqIndexer= 0;
		bool hasComment= false;
		vector<ffmap::iterator>* itVecPtr= fmMapSequence.m_pvpsMapSequence;
		if (itVecPtr) iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
		else i= objmap.begin();			
		TxjPrettyPrintPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<Txj_*> (this);
		lfpo.m_msviClWidths= &msviClWidths;
		int iLastNwLnIndex= 0;
		if (isEFlagSet(EXT_VIA_PARENT)) {
			if (isEFlagSet(EXTENDED)) {
				iLastNwLnIndex= 1;
			} else {
				flErr(TXJ_MAIN, "An object never extends via parent!");
				return;
			}
		} else if (isEFlagSet(HAS_CHILDREN)) {

		} else {
		}
		ps+= (save && isEFlagSet(CASTFILE))? "" : string(1, opnBrc);
		if (!(save && isEFlagSet(CASTFILE)))
			ps+= '\n';
		while (1) {
			uint32_t t= (i->second || i->second->isType(LINK)) ?
				i->second->getType() : NUL;
			bool isComment= i->first[0]=='#';
			bool printComment= (printComments) && isComment;
			if (t==UNDEFINED || (isComment && !printComment)) {
				goto prtyPrntIter;
			}
			if (printComment && ps[ps.size()-2]!='\n')
				ps+= '\n';
			if (isEFlagSet(B64ENCODE))i->second->setEFlag(B64ENCODE);
			if ((isEFlagSet(B64ENCODE_CHILDREN))&&!isEFlagSet(B64ENCODE_STOP))
				i->second->setEFlag(B64ENCODE_CHILDREN);
			ps.append(indent+1, '\t');
			if (json) ps+= "\"";
			ps+= i->first;
			lfpo.name= &i->first;
			if (json) ps+= "\"";
			ps+= ": ";
			i->second->prettyString(
				ps, json, printComments, indent+1, &lfpo, printFilePath, save);
		  prtyPrntIter:
			iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
			if (i==objmap.end()) {
				if (t==UNDEFINED) {
					ps.pop_back();
					ps.back()= '\n';
				} else {
					ps+= '\n';
				}
				if (!(save && isEFlagSet(CASTFILE))){
					ps.append(indent>0?indent:0, '\t');
					ps+= clsBrc;
				}
				break;
			} else if (t!= UNDEFINED) {
				if (!isComment || printComment)
					ps.append(",\n");
				if (hasComment && (save || printComments)) {
					ps+= '\n';
					hasComment= false;
				}
				if (isComment) hasComment= true;
			}
		};
		break;
	}
	case SET_TYPE: {
		ffset& objset= *(val.setPtr);
		TxjPrettyPrintPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<Txj_*> (this);
		ps+= (save && isEFlagSet(CASTFILE))? "" : "{";
		if(objset.size()) {
			for (Txj_* fp : objset) {
				fp->prettyString(ps, json, printComments, indent+1, &lfpo,
									  printFilePath, save);
				ps+= ',';
			}
			ps.pop_back();
		}
		if (!(save && isEFlagSet(CASTFILE)))
			ps+= '}';
		break;
	}
	case ARRAY: {
		vector<Txj_*>& objarr= *(val.array);
		int iLastNwLnIndex= 0;
		vector<int> vClWidths;
		map<string, vector<int> > msviClWidths;
		TxjPrettyPrintPObj lfpo;
		lfpo.pObj= pObj;
		lfpo.value= const_cast<Txj_*> (this);
		lfpo.m_msviClWidths= &msviClWidths;
		int iParentHeight= 0;
		if (!size) {
			ps+= "[]"; break;
		}
		// if (isEFlagSet(EXT_VIA_PARENT)) {
		// 	if (isEFlagSet(EXTENDED)) {
		// 		ps= (save && isEFlagSet(CASTFILE))? "": "[\n";
		// 		iLastNwLnIndex= 1;
		// 	} else {
		// 		iParentHeight= 1;
		// 		TxjPrettyPrintPObj* pFFPPPObjHolder=
		// 			static_cast<TxjPrettyPrintPObj*> (pObj->pObj);
		// 		string sChildName= *pFFPPPObjHolder->name;
		// 		while (pFFPPPObjHolder && pFFPPPObjHolder->m_msviClWidths->
		// 				 find(sChildName)==pFFPPPObjHolder->m_msviClWidths->end()) {
		// 			pFFPPPObjHolder= static_cast<TxjPrettyPrintPObj*>
		// 				(pFFPPPObjHolder->pObj);
		// 			if (pFFPPPObjHolder)
		// 				sChildName= *pFFPPPObjHolder->name+'.'+sChildName;
		// 			++iParentHeight;
		// 		}
		// 		if (!pFFPPPObjHolder) {
		// 			flErr(TXJ_MAIN, "ColumnWidths not found");
		// 			return "";
		// 		}
		// 		vClWidths= (*pFFPPPObjHolder->
		// 						m_msviClWidths)[sChildName];
		// 		ps= (save && isEFlagSet(CASTFILE))?"":"[";
		// 		int iNameLength= 0;
		// 		if (pObj && pObj->value->isType(OBJ)) {
		// 			iNameLength= pObj->name->length()+3;
		// 		}
		// 		ps.append((((vClWidths[0]+7) / 8)*8-8 * iParentHeight -
		// 					  iNameLength+7) / 8, '\t');
		// 		lfpo.m_bGiveFirstLine= true;
		// 	}
		// } else if (isEFlagSet(HAS_CHILDREN)) {
		// 	ps= (save && isEFlagSet(CASTFILE))?"":"[";
		// 	vector<Txj_*>* pvpfjChildren= getFeaturedMember(FM_CHILDREN).
		// 		m_pvChildren;
		// 	if (pvpfjChildren->size()>0) {
		// 		vClWidths.resize(size+1, 0);
		// 		vClWidths[0]= pObj->name->length()+3;
		// 		for (int i= 0; i<pvpfjChildren->size(); ++i) {
		// 			iLastNwLnIndex= 1;
		// 			map<string, int>& mTabHead= *(*pvpfjChildren)[i]->val.fptr
		// 				->getFeaturedMember(FM_TABHEAD).tabHead;
		// 			map<string, int>::iterator itTabHead= mTabHead.begin();
		// 			Txj_* pfjChild= (*pvpfjChildren)[i]->val.fptr;
		// 			ffmap::iterator itmspfTrueChild;
		// 			vector<string>* vsLink= (*pvpfjChildren)[i]->
		// 				getFeaturedMember(FM_LINK).link;
		// 			while (itTabHead!=mTabHead.end()) {
		// 				int iCurColWidth= itTabHead->first.size()+3;
		// 				int iCurColIndex= itTabHead->second;
		// 				vClWidths[iCurColIndex+1]= iCurColWidth;
		// 				if (pfjChild->isType(OBJ))
		// 					itmspfTrueChild= pfjChild->val.pairs->begin();
		// 				for (int j= 0; j<pfjChild->size; ++j) {
		// 					Txj_* pfjChildMem;
		// 					if (pfjChild->isType(ARRAY)) {
		// 						pfjChildMem= (*(*pfjChild->val.array)[j]->val.array)
		// 							[iCurColIndex];
		// 					} else if (pfjChild->isType(OBJ)) {
		// 						pfjChildMem= (*itmspfTrueChild->second->val.array)
		// 							[iCurColIndex];
		// 						++itmspfTrueChild;
		// 					}
		// 					unsigned int width= pfjChildMem->getFeaturedMember
		// 						(FM_WIDTH).width;
		// 					if (pfjChildMem->isType(STRING)) {
		// 						if (pfjChildMem->isEFlagSet(LONG_LAST_LN))
		// 							width+= 3;
		// 						else if (pfjChildMem->isEFlagSet(ONE_SHORT_LAST_LN))
		// 							width+= 2;
		// 						else
		// 							width+= 1;
		// 					} else {
		// 						width+= 1;
		// 					}
		// 					if (iCurColWidth<width) {
		// 						iCurColWidth= width;
		// 						vClWidths[iCurColIndex+1]= width;
		// 					}
		// 				}
		// 				++itTabHead;
		// 			}
		// 			if (pfjChild->isType(OBJ)) {
		// 				itmspfTrueChild= pfjChild->val.pairs->begin();
		// 				while (itmspfTrueChild!=pfjChild->val.pairs->end()) {
		// 					int iCurChldInitIndent= (vsLink->size())*8 +
		// 						itmspfTrueChild->first.length()+3;
		// 					if (iCurChldInitIndent>vClWidths[0])
		// 						vClWidths[0]= iCurChldInitIndent;
		// 					++itmspfTrueChild;
		// 				}
		// 			}
		// 			// set the initial indent in 0th index
		// 		}
		// 		for (int i= 0; i<pvpfjChildren->size(); ++i) {
		// 			vector<string>* vsLink= (*pvpfjChildren)[i]->
		// 				getFeaturedMember(FM_LINK).link;
		// 			string sChild= implode(".", (*vsLink));
		// 			(*pObj->m_msviClWidths)[sChild]= vClWidths;
		// 		}
		// 	}
		// 	lfpo.m_bGiveFirstLine= true;
		// 	ps.append((((vClWidths[0]+7)/8)*8-pObj->name->length()+7)/8, '\t');
		// } else {
			ps+= (save && isEFlagSet(CASTFILE))?"":"[\n";
		// }
		int i= 0;
		bool bInCompleteStrs= false;
		vector<Txj_*> vpfjMulLnStrs;
		if (!(save && isEFlagSet(CASTFILE)))ps.append(indent+1, '\t');
		while (i<objarr.size()) {
			uint32_t t= objarr[i] ? objarr[i]->getType() : NUL;
			if (t!=UNDEFINED && t!=NUL) {
				if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
				if ((isEFlagSet(B64ENCODE_CHILDREN))&&
					 !isEFlagSet(B64ENCODE_STOP))
					objarr[i]->setEFlag(B64ENCODE_CHILDREN);
				string ind= to_string(i);
				lfpo.name= &ind;
				if (objarr[i]->isType(STRING) &&
					 (isEFlagSet(HAS_CHILDREN) ||
					  isEFlagSet(EXT_VIA_PARENT))) {
				}
				objarr[i]->prettyString(
					ps, json, printComments, indent+1, &lfpo, printFilePath, save);
			} else if (t==NUL) {
				//ps.append(indent+1, '\t');
			}
			//int width= sMem.length();
			// if ((isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) ||
			// 	 isEFlagSet(HAS_CHILDREN)
			// ) {
			// 	bInCompleteStrs |= !lfpo.m_bGiveFirstLine;
			// 	if (lfpo.m_bGiveFirstLine) {
			// 		vpfjMulLnStrs.push_back(NULL);
			// 		if (i+1!=objarr.size()) {
			// 			ps+= ',';
			// 			width++;
			// 		}
			// 	} else {
			// 		lfpo.m_bGiveFirstLine= true;
			// 		vpfjMulLnStrs.push_back(objarr[i]);
			// 	}
			// 	ps.append((((vClWidths[i+1]+(((i+1)<objarr.size()) ? 8 :
			// 										  6)) / 8)*8-width+7) / 8, '\t');
			// } else {
				if (i+1!=objarr.size()) {
					ps.append(", ");
				} else {
					ps.append("\n");
				}
			// }
			if (++i!=objarr.size()) {

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
				ConstructMultiLineStringArray(vpfjMulLnStrs, indent, vClWidths));
		}
		// if ((isEFlagSet(EXT_VIA_PARENT) && !isEFlagSet(EXTENDED)) ||
		// 	 isEFlagSet(HAS_CHILDREN)) {
		// } else
		if (!(save && isEFlagSet(CASTFILE))) {
			ps.append(indent, '\t');
			ps.append("]");
		}
		break;
	}
	case LINK: {
		vector<string>* vtProp= getFeaturedMember(FM_LINK).link;
		if (save || returnNameIfDeclared(*vtProp, pObj)!=NULL) {
			string ln= implode(".", *vtProp);
			ps+=  json?"\""+ln+"\"":ln;
			return;
		} else {
			return val.fptr->prettyString(
				ps, json, printComments, indent, pObj, printFilePath, save);
		}
		break;
	}
	case DLINK: {
		return val.fptr->prettyString(
			ps, json, printComments, indent+1, pObj, printFilePath, save);
	}
	case BINARY: {
		ps+= "("+to_string(size)+")";
		ps.append((const char*) val.vptr, size);
		break;
	}
	case TIME:
		ps+= (string)(*val.m_pFerryTimeStamp);
		return;
	default:
		if (!isQType(NONE)) {
			if (isQType(QUERY)) {
				ps+= '?';
				return;
			} else if (isQType(DEL)) {
				ps+= "delete";
				return;
			}
		} else {
			ps+= json?"null":"";
		}
	}
	if (isEFlagSet(EXTENDED) && !isType(STRING)) {
		Txj_* pParent= getFeaturedMember(FM_PARENT).m_pParent;
		ps+= " | ";
		pParent->stringify(ps, false, false, pObj);
	}
}

string Txj_::ConstructMultiLineStringArray (
	vector<Txj_*>& vpfMulLnStrs, int indent, vector<int>& vClWidths
) const {
	string sProduct;
	int iLineIndex= 1;
	bool bAreMulLnStrsRemained= false;
	do {
		bAreMulLnStrsRemained= false;
		sProduct+= '\n';
		sProduct.append(indent+((vClWidths[0]-8+7) / 8), '\t');
		for (int i= 0; i<vpfMulLnStrs.size(); ++i) {
			if (vpfMulLnStrs[i]) {
				int iCurLnInd= 0;
				char* pcNwLn= vpfMulLnStrs[i]->val.str;
				while (iCurLnInd<iLineIndex && pcNwLn) {
					pcNwLn= strchr(pcNwLn, '\n');
					if (pcNwLn)pcNwLn++;
					iCurLnInd++;
				}
				char* pcNxtLn= NULL;
				if (pcNwLn)pcNxtLn= strchr(pcNwLn, '\n');
				int width= 0;
				if (pcNxtLn) {
					sProduct+= ' ';
					width= pcNxtLn-pcNwLn;
					sProduct.append(pcNwLn, width);
					sProduct.append((((vClWidths[i+1]+8) / 8)*8-(int)
										  width-1+7) / 8, '\t');
					bAreMulLnStrsRemained |= true;
				} else if (pcNwLn) {
					pcNxtLn= pcNwLn;
					while (*pcNxtLn)pcNxtLn++;
					sProduct+= ' ';
					sProduct.append(pcNwLn);
					int cwidth=
						((vClWidths[i+1]+(((i+1)<vpfMulLnStrs.size())?8:6))/8)*8-
						(int)(pcNxtLn-pcNwLn)-3+7;
					if (i<vpfMulLnStrs.size()-1) {
						sProduct+= "\",";
						sProduct.append(cwidth/8, '\t');
					} else {
						sProduct+= '"';
						sProduct.append((cwidth+1)/8, '\t');
					}
					vpfMulLnStrs[i]= NULL;
				}
			} else {
				sProduct.append((vClWidths[i+1]+(((i+1)<vpfMulLnStrs.
																  size()) ? 8 : 6)) / 8, '\t');
			}
		}
		iLineIndex++;
	} while (bAreMulLnStrsRemained);
	return sProduct;
}

Txj_::operator const char* () {
	return isLink() ? val.fptr->val.str : val.str;
}

Txj_::operator double () {
	return isLink() ? val.fptr->val.number : val.number;
}

Txj_::operator float () {
	return (float) isLink() ?
		val.fptr->val.number : (isType(STRING)? atof(val.str): val.number);
}

Txj_::operator bool () {
	Txj_* fp= this;
	if (isLink()) {
		fp= val.fptr;
	};
	OBJ_TYPE t= fp->getType();
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

Txj_::operator int () {
	if (isLink()) {
		return (int) (val.fptr->val.number);
	}
	return (int) val.number;
}

Txj_::operator long () {
	if (isLink()) {
		return (long) (val.fptr->val.number);
	}
	return (long) val.number;
}

Txj_::operator unsigned int () {
	if (isLink()) {
		return (unsigned int) (val.fptr->val.number);
	}
	return (unsigned int) val.number;
}

Txj_& Txj_::operator= (Blob_ b) {
	if (isQType(UPDATE)) {
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	Txj_* parent= nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent= val.fptr;
	}
	freeObj(true);
	setType(BINARY);
	size= b.s;
	val.vptr= b.p;
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			return nullTxj;
		}
	}
	return *this;
}
// Txj_& Txj_::operator = (char* s) {
// 	return (*this)= (ccp)s;
// }
Txj_& Txj_::operator = (const char* s) {
	lock();
	if (isQType(UPDATE)) {
		FeaturedMember fm= getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	Txj_* parent= nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent= val.fptr;
	}
	freeObj(true);
	int i= 0;
	int j= strlen(s);
	if (s[0]=='<' && s[j-1]=='>') {
		++i;
		int xmlNail= i;
		string xmlTag;
		int length= -1;
		bool tagset= false;
		while (s[i]!='>' && i<j) {
			if (s[i]==' ') {
				tagset= true;
				if (s[i+1]=='l' && s[i+2]=='e' && s[i+3]=='n' &&
					 s[i+4]=='g' && s[i+5]=='t' && s[i+6]=='h') {
					i+= 7;
					while (s[i]!='=' && i<j) {
						++i;
					}
					++i;
					while (s[i]!='"' && i<j) {
						++i;
					}
					++i;
					string lengthstr;
					while (s[i]!='"' && i<j) {
						lengthstr+= s[i];
						++i;
					}
					length= atoi(lengthstr.c_str());
				}
			} else if (!tagset) {
				xmlTag+= s[i];
			}
			++i;
		}
		setType(XML);
		++i;
		xmlNail= i;
		if (length>-1 && length<(j-i)) {
			i+= length;
		}
		while (i<j) {
			if (s[i]=='<' &&
				 s[i+1]=='/') {
				if (xmlTag.compare(0, xmlTag.length(), s+i+2, xmlTag.length())
					==0 && s[i+2+xmlTag.length()]=='>') {
					size= i-xmlNail;
					val.str= new char[size+1];
					memcpy(val.str, s+xmlNail,
							 size);
					val.str[size]= '\0';
					i+= 3+xmlTag.length();
					break;
				}
			}
			++i;
		}
	} else {
		setType(STRING);
		size= strlen(s);
		val.str= new char[size+1];
		//int iLastNewLnIndex= 0;
		//FeaturedMember fmWidth;
		//fmWidth.width= 0;
		int i= 0;
		for (i= 0; i<size; ++i) {
			// if (s[i]=='\n') {
			// 	if (i-iLastNewLnIndex>fmWidth.width)
			// 		fmWidth.width= i-iLastNewLnIndex;
			// 	iLastNewLnIndex= i;
			// }
			val.str[i]= s[i];
		}
		// if (i-iLastNewLnIndex>=fmWidth.width) {
		// 	fmWidth.width= i-iLastNewLnIndex;
		// 	setEFlag(LONG_LAST_LN);
		// } else if (i-iLastNewLnIndex==fmWidth.width-1) {
		// 	setEFlag(ONE_SHORT_LAST_LN);
		// }
		// insertFeaturedMember(fmWidth, FM_WIDTH);
		val.str[size]= '\0';
	}
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			unlock();
			return nullTxj;
		}
	}
	unlock();
	return *this;
}

Txj_& Txj_::operator = (const string& s) {
	operator=(s.c_str());
	return *this;
}

Txj_& Txj_::operator = (const int& i) {
	lock();
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	Txj_* parent= nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent= val.fptr;
	}
	freeObj(true);
	setType(NUMBER);
	val.number= i;
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			unlock();
			return nullTxj;
		}
	}
	unlock();
	return *this;
}

Txj_& Txj_::operator = (const Txj_& f) {
	lock();
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	Txj_* parent= nullptr;
	if (isType(NEW_SET_MEMBER)) {
		parent= val.fptr;
	}
	if (f.isType(UNDEFINED)) {
		freeObj();
		setType(UNDEFINED);
		unlock();
		return *this;
	}
	if (!((isType(OBJ) || isType(ORDERED_OBJ)) &&
			(f.isType(OBJ) || f.isType(ORDERED_OBJ))))
		freeObj(true);
	unlock();
	f.lockShared();
	copy(f, COPY_ALL);
	f.unlockShared();
	if (parent) {
		if (parent->val.setPtr->insert(this).second)
			++parent->size;
		else {
			delete this;
			return nullTxj;
		}
	}
	return *this;
}

// need to implement, segfaults during stringify but
// can be used to hold pointers
Txj_& Txj_::operator= (Txj_* f) {
	if (this==f)
		return *this;
	if (isQType(UPDATE)) {
		FeaturedMember fm= getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	lock();
	freeObj(true);
	setType(DLINK);
	val.fptr= f;
	unlock();
	return *this;
}

shared_mutex& Txj_::getMtxMapMtx () const {
	MtxMapMtx.lock_shared();
	map<const Txj_*, shared_mutex>::iterator it= MtxMap.find(this);
	MtxMapMtx.unlock_shared();
	if (it==MtxMap.end()) {
		lock_guard<shared_mutex> lk(MtxMapMtx);
		return MtxMap[this];
	} else {
		return it->second;
	}
}

void Txj_::lock () const {
	shared_mutex& mtx= getMtxMapMtx();
	mtx.lock();
}

void Txj_::unlock () const {
	shared_mutex& mtx= getMtxMapMtx();
	mtx.unlock();
}
void Txj_::lockShared () const {
	shared_mutex& mtx= getMtxMapMtx();
	mtx.lock_shared();
}
void Txj_::unlockShared () const {
	shared_mutex& mtx= getMtxMapMtx();
	mtx.unlock_shared();
}
bool Txj_::tryLock () const {
	shared_mutex& mtx= getMtxMapMtx();
	return mtx.try_lock();
}
bool Txj_::tryLockShared () const {
	shared_mutex& mtx= getMtxMapMtx();
	return mtx.try_lock_shared();
}

void Txj_::prune () {
	MtxMapMtx.lock();
	size_t mpsize= MtxMap.size();
	map<const Txj_*, shared_mutex>::iterator it= MtxMap.begin();
	if (mpsize>50) {
		while (it!= MtxMap.end()) {
			map<const Txj_*, shared_mutex>::iterator iit= it++;
			if (iit->second.try_lock()) {
				MtxMap.erase(iit);
			};
		}
	}
	MtxMapMtx.unlock();
}

Txj_& Txj_::addLink (const Txj_& PObj, string label) {
	vector<string>* prop= new vector<string>();
	explode(".", label, *prop);
	Txj_* obj= const_cast<Txj_*>(PObj.returnNameIfDeclared(*prop));
	if (obj) {
		Txj_* parent= nullptr;
		if (!isType(NEW_SET_MEMBER))
			freeObj(true);
		else {
			parent= val.fptr;
		}
		setType(LINK);
		val.fptr= obj;
		FeaturedMember cFM;
		cFM.link= prop;
		insertFeaturedMember(cFM, FM_LINK);
		if (parent) {
			if (parent->val.setPtr->insert(this).second)
				++parent->size;
			else {
				delete this;
				return nullTxj;
			}
		}
	} else {
		delete prop;
	}
	return *this;
}

Txj_& Txj_::addLink (const string&& objPath, const string&& linkPath) {
	vector<string>* objProp= new vector<string>();
	explode(".", objPath, *objProp);
	Txj_* obj= const_cast<Txj_*>(this->returnNameIfDeclared(*objProp));
	Txj_* link= this;
	if (obj) {
		Txj_* parent= NULL;
		vector<string>* linkProp= new vector<string>();
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
			parent= link->val.fptr;
		}
		link->setType(LINK);
		link->val.fptr= obj;
		FeaturedMember cFM;
		cFM.link= objProp;
		link->insertFeaturedMember(cFM, FM_LINK);
		if (parent) {
			if (parent->val.setPtr->insert(link).second)
				++parent->size;
			else {
				delete link;
				link=&nullTxj;
			}
		}
		delete linkProp;
		return *link;
	} else {
		delete objProp;
	}
	return *link;
}

Txj_& Txj_::operator = (const double& d) {
	if (isQType(UPDATE)) {
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	FeaturedMember cFM;
	cFM.precision= 16;
	setEFlag(PRECISION);
	insertFeaturedMember(cFM, FM_PRECISION);
	setType(NUMBER);
	val.number= d;
	return *this;
}

Txj_& Txj_::operator = (const float& f) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	FeaturedMember cFM;
	cFM.precision= 8;
	setEFlag(PRECISION);
	insertFeaturedMember(cFM, FM_PRECISION);
	setType(NUMBER);
	val.number= f;
	return *this;
}

Txj_& Txj_::operator = (const long& l) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number= l;
	return *this;
}

Txj_& Txj_::operator = (const short& s) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number= s;
	return *this;
}

Txj_& Txj_::operator = (const unsigned int& i) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(NUMBER);
	val.number= i;
	return *this;
}

Txj_& Txj_::operator = (const bool& b) {
	if(isQType(UPDATE)){
		FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
		fm.m_pTimeStamp->update();
	}
	freeObj(true);
	setType(BOOL);
	val.boolean= b;
	return *this;
}
void Txj_::trim () {
	if (isType(ORDERED_OBJ)) {
		int i;
		FeaturedMember fmMapSeq= getFeaturedMember(FM_MAP_SEQUENCE);
		vector<ffmap::iterator >* vmpspfPairs=
			fmMapSeq.m_pvpsMapSequence;
		i= size-1;
		while (i>=0 && vmpspfPairs) {
			if (((*(*vmpspfPairs)[i]->second).isType(UNDEFINED)
				  &&!(*(*vmpspfPairs)[i]->second).isQType(NONE)) ||
				 (*(*vmpspfPairs)[i]->second).isType(NUL)) {
				delete (*vmpspfPairs)[i]->second;
				val.pairs->erase((*vmpspfPairs)[i]);
				vmpspfPairs->erase(vmpspfPairs->begin()+i);
				--size;
			}
			--i;
		}
	} else if (isType(ARRAY)) {
		if ((*this)[size-1].isType(UNDEFINED)) {
			delete (*val.array)[size-1];
			val.array->pop_back();
			--size;
		}
		int i= 0;
		while (i<val.array->size()) {
			if ((*val.array)[i]!=NULL) {
				if (((*val.array)[i]->isType(UNDEFINED)&&
					  !(*val.array)[i]->isQType(NONE)) ||
					 (*val.array)[i]->isType(NUL)) {
					delete (*val.array)[i];
					(*val.array)[i]= NULL;
				}
			}
			++i;
		}
	}
}

string Txj_::queryString () {
	if (isType(STRING)) {
		if (isQType(QUERY_TYPE::SET)) {
			return ("\""+string(val.str, size)+"\"");
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(NUMBER)) {
		if (isQType(QUERY_TYPE::SET)) {
			return to_string(val.number);
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(XML)) {
		if (isQType(QUERY_TYPE::SET)) {
			if (isEFlagSet(B64ENCODE)) {
				int output_length= 0;
				char * b64_char= base64_encode((const unsigned char*)
														  val.str, size, (size_t*) & output_length);
				string b64_str(b64_char, output_length);
				free(b64_char);
				return ("\""+b64_str+"\"");
			} else {
				return ("<xml length=\""+to_string(size)+"\">" +
						  string(val.str, size)+"</xml>");
			}
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(BOOL)) {
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
	} else if (isType(OBJ)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			string ffs;
			ffmap& objmap= *(val.pairs);
			ffs= "{";
			ffmap::iterator i;
			i= objmap.begin();
			bool matter= false;
			while (i!=objmap.end()) {
				string ffjsonStr;
				uint32_t t= (i->second!=NULL) ? i->second->getType() : NUL;
				if (t!=UNDEFINED || (t!=NUL && !i->second->isQType(NONE))) {
					if (t!=NUL) {
						if (isEFlagSet(B64ENCODE))i->second->
															  setEFlag(B64ENCODE);
						if ((isEFlagSet(B64ENCODE_CHILDREN))&&
							 !isEFlagSet(B64ENCODE_STOP))
							i->second->setEFlag(B64ENCODE_CHILDREN);
						ffjsonStr= i->second->queryString();
					}
					if (ffjsonStr.length()>0) {
						if (matter)ffs.append(",");
						ffs.append("\""+i->first+"\":");
						ffs.append(ffjsonStr);
						matter= true;
					}
				}
				++i;
			}
			if (ffs.length()==1) {
				ffs= "";
			} else {
				ffs+= '}';
			}
			return ffs;
		}
	} else if (isType(ARRAY)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DEL)) {
			return "delete";
		} else {
			string ffs;
			vector<Txj_*>& objarr= *(val.array);
			ffs= "[";
			bool matter= false;
			int i= 0;
			string ffjsonstr;
			bool firstTime= false;
			while (i<objarr.size()) {
				uint32_t t= objarr[i]!=NULL ? objarr[i]->getType() : NUL;
				if (t!=UNDEFINED || (t!=NUL&&!objarr[i]->isQType(NONE))) {
					if (t!=NUL) {
						if (isEFlagSet(B64ENCODE))objarr[i]->setEFlag(B64ENCODE);
						if ((isEFlagSet(B64ENCODE_CHILDREN))&&
							 !isEFlagSet(B64ENCODE_STOP))
							objarr[i]->setEFlag(B64ENCODE_CHILDREN);
						ffjsonstr= objarr[i]->queryString();
					} else {
						ffjsonstr= "";
					}
					if (firstTime)ffs+= ',';
					firstTime= true;
					if (ffjsonstr.length()>0) {
						ffs.append(ffjsonstr);
						matter= true;
					}
				}
				++i;
			}
			if (matter) {
				ffs+= ']';
			} else {
				ffs= "";
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

Txj_* Txj_::answerObject (
	Txj_* queryObject, TxjPObj* pObj, FerryTimeStamp lastUpdateTime, Txj_* ao
) {
	Txj_* fp= this;
	if (isLink()) {
		fp= fp->val.fptr;
	}
	TxjPObj ffpThisObj;
	ffpThisObj.value= this;
	ffpThisObj.pObj= pObj;
	if (queryObject->isQType(UPDATE)) {
		if (queryObject->isType(UNDEFINED)) {
			if (fp->isType(OBJ))
				queryObject->init("{}");
			else if (fp->isType(ARRAY))
				queryObject->init("[]");
			else
				queryObject->setQType(QUERY);
		}
	} else if (sm_mUpdateObjs.find(this)!=sm_mUpdateObjs.end()) {
		if (queryObject->isType(UNDEFINED)) {
			if (fp->isType(OBJ))
				queryObject->init("{}");
			else if (fp->isType(ARRAY))
				queryObject->init("[]");
		}
		if (fp->isQType(UPDATE)) {
			if (!queryObject->isQType(DEL)) {
				if (pObj->value->isType(OBJ)) {
					Txj_::Iterator itUpdateTime=
						pObj->value->find(*pObj->name);
					++itUpdateTime;
					if (itUpdateTime.getIndex().find("(Time)")==0) {
						if (((FerryTimeStamp&)(*itUpdateTime)>=lastUpdateTime)) {
							if(queryObject->isType(OBJ)||queryObject->isType(ARRAY))
								queryObject->setQType(UPDATE);
							else
								queryObject->setQType(QUERY);
						}
					} else {
						if(queryObject->isType(OBJ)||queryObject->isType(ARRAY))
							queryObject->setQType(UPDATE);
						else
							queryObject->setQType(QUERY);
					}
				}
			}
		} else if (sm_mUpdateObjs[this].size()>0) {
			if (queryObject->isType(OBJ)) {
				set<Txj_::TxjIterator>& lsObjs= sm_mUpdateObjs[this];
				set<Txj_::TxjIterator>::iterator i= lsObjs.begin();
				while (i!=lsObjs.end()) {
					(*queryObject)[i->m_itMap->first];
					++i;
				}
			} else if (queryObject->isType(ARRAY)) {

			}
		}
	}
	if (queryObject->isQType(DEL)) {
		fp->freeObj();
		setType(NUL);
	} else if (queryObject->isQType(QUERY)) {
		ao= new Txj_(*this);
	} else if (queryObject->isType(fp->getType())) {
		if (queryObject->isQType(NQUERY)) {
			Txj_& rao= *ao;
			ffmap::iterator it= fp->val.pairs->begin();
			while (it!=fp->val.pairs->end()) {
				ffmap::iterator fit=
					queryObject->val.pairs->find(it->first);
				if (fit==queryObject->val.pairs->end()) {
					rao[it->first]= it->second;
				}
				++it;
			}
			return nullptr;
		}
		if (queryObject->isType(OBJ) || queryObject->isType(ORDERED_OBJ)) {
			ffmap::iterator i, j;
			FeaturedMember fmMapSequence, fmOrigMapSequence;
			ffmap& objmap= *queryObject->val.pairs;
			fmMapSequence= queryObject->getFeaturedMember(FM_MAP_SEQUENCE);
			int iMapSeqIndexer= 0, iOrigMapSeqIndexer= 0;
			ffmap::iterator itEnd;
			vector<ffmap::iterator>* itVecPtr= fmMapSequence.m_pvpsMapSequence;
			if (itVecPtr) iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
			else i= objmap.begin();			
			itEnd= queryObject->val.pairs->end();
			j= itEnd;
			if (!queryObject->isQType(UPDATE))
				goto skiporig;
			j= i;
			fmOrigMapSequence= fmMapSequence;
			fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
			itVecPtr= fmMapSequence.m_pvpsMapSequence;
			if (itVecPtr) iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
			else i= objmap.begin();			
			itEnd= fp->val.pairs->end();
		  skiporig:
			FeaturedMember fmAOMapSequence;
			while (i!=itEnd) {
				ffmap::iterator k;
				Txj_* lao= NULL;
				ffpThisObj.name= &i->first;
				Txj_ tempUpdtObj("^");
				if (queryObject->isQType(UPDATE)) {
					if (i->first==j->first) {
						j->second->setQType(UPDATE);
						lao= i->second->answerObject(j->second, &ffpThisObj,
															  lastUpdateTime);
					} else {
						lao= i->second->answerObject(&tempUpdtObj,
															  &ffpThisObj, lastUpdateTime);
					}
				} else if ((k= fp->val.pairs->find(i->first)) !=
							  fp->val.pairs->end()) {
					lao= k->second->answerObject(i->second, &ffpThisObj,
														  lastUpdateTime);
				} else {
					/*Txj_* nao= new Txj_(*i->second);
					  if (!nao->isType(UNDEFINED)) {
					  (*this).val.pairs[i->first]= nao;
					  }*/
				}
				if (lao!=NULL) {
					if (ao==NULL)ao= new Txj_(queryObject->getType());
					fmAOMapSequence= ao->getFeaturedMember(FM_MAP_SEQUENCE);
					pair <ffmap::iterator, bool> prNew=
						ao->val.pairs->insert(pair<string, Txj_*>(i->first, lao));
					if (fmAOMapSequence.m_pvpsMapSequence) {
						fmAOMapSequence.m_pvpsMapSequence->push_back(prNew.first);
					}
				}
				if (j!=itEnd && i->first==j->first) {
					if (fmOrigMapSequence.m_pvpsMapSequence) {
						if (iOrigMapSeqIndexer<fmOrigMapSequence.
							 m_pvpsMapSequence->size()) {
							j= (*fmOrigMapSequence.m_pvpsMapSequence)
								[iOrigMapSeqIndexer++];
						} else {
							j= queryObject->val.pairs->end();
						}
					} else {
						++j;
					}
				}
				iterSeq(itVecPtr, i, iMapSeqIndexer, objmap);
			}
			if (queryObject->isQType(UPDATE) && j!=queryObject->val.pairs->end())
			{
				flErr(TXJ_MAIN, "QueryObject order didn't match. QueryObject:%s",
						queryObject->stringify().c_str());
			}
		} else if (queryObject->isType(ARRAY)) {
			if (queryObject->size==size) {
				int i= 0;
				bool matter= false;
				ao= new Txj_("[]");
				while (i<size) {
					if ((*queryObject->val.array)[i]!=NULL ||
						 queryObject->isQType(UPDATE)) {
						Txj_* ffo= NULL;
						string ind= to_string(i);
						ffpThisObj.name= &ind;
						Txj_ tempUpdtObj("^");
						if ((*queryObject->val.array)[i]==NULL) {
							ffo= (*fp->val.array)[i]->answerObject
								(&tempUpdtObj, &ffpThisObj, lastUpdateTime);
						} else {
							ffo= (*fp->val.array)[i]->answerObject
								((*queryObject->val.array)[i], &ffpThisObj,
								 lastUpdateTime);
						}
						if (ffo) {
							ao->val.array->push_back(ffo);
							matter= true;
						} else {
							ao->val.array->push_back(NULL);
						}
					} else {
						ao->val.array->push_back(NULL);
					}
					++i;
				}
				if (!matter) {
					delete ao;
					ao= NULL;
				}
			}
		} else if (queryObject->isType(STRING) || queryObject->isType(XML) ||
					  queryObject->isType(BOOL) || queryObject->isType(NUMBER) ||
					  queryObject->isType(TIME)) {
			freeObj();
			copy(*queryObject);
		} else {
			ao= NULL;
		}
	}
	return ao;
}

//bool Txj_::isType(uint8_t t) const {
//
//	  return (t==type);
//}

bool Txj_::isType (OBJ_TYPE t) const {
	return (t==(uint8_t)(flags & 0x000000ff));
}

bool Txj_::isLink () const {
	OBJ_TYPE t= (OBJ_TYPE)(flags & 0x000000ff);
	return (LINK==t||DLINK==t);
}


//void Txj_::setType(uint8_t t) {
//
//	  type= t;
//}

void Txj_::setType (OBJ_TYPE t) {
	flags &= 0xffffff00;
	flags |= t;
}

//uint8_t Txj_::getType() const {
//
//	  return type;
//}

Txj_::OBJ_TYPE Txj_::getType () const {
	uint32_t type= 0xff;
	type &= flags;
	return (OBJ_TYPE) type;
}

//bool Txj_::isQType(uint8_t t) const {
//
//	  return (t==qtype);
//}

bool Txj_::isQType (QUERY_TYPE t) const {
	uint32_t qtype= flags;
	qtype &= 0xff00;
	return (t==qtype);
}

//void Txj_::setQType(uint8_t t) {
//			 
//	  qtype= t;
//}

void Txj_::setQType (QUERY_TYPE t) {
	flags &= (~0xff00);
	flags |= t;
}

//uint8_t Txj_::getQType() const {
//
//	  return qtype;
//}

Txj_::QUERY_TYPE Txj_::getQType () const {
	uint32_t qtype= flags;
	qtype &= 0xff00;
	return (QUERY_TYPE) qtype;
}

//bool Txj_::isEFlagSet(int t) const {
//
//	  return (t & etype==t);
//}

bool Txj_::isEFlagSet (E_FLAGS t) const {
	return (t & flags);
}

//uint8_t Txj_::getEFlags() const {
//
//	  return this->etype;
//}

Txj_::E_FLAGS Txj_::getEFlags () const {
	return (E_FLAGS) (flags & 0x0fff0000);
}

//void Txj_::setEFlag(int t) {
//		
//	  etype |= t;
//}

void Txj_::setEFlag (E_FLAGS t) const {
	flags |= t;
}

//void Txj_::clearEFlag(int t) {
//
//	  etype &= ~t;
//}

void Txj_::clearEFlag (E_FLAGS t) {
	flags &= ~(t);
}

void Txj_::erase (string name) {
	Txj_* fp= this;
	if (isLink())fp= val.fptr;
	if (fp->isType(OBJ) || fp->isType(ORDERED_OBJ)) {
		// find MUST be inside the lock: an unlocked find can return an
		// iterator to a node another thread erases before we lock, and
		// dereferencing it below would be a use-after-free
		fp->lock();
		ffmap::iterator it= fp->val.pairs->find(name);
		if (it==fp->val.pairs->end()) {
			fp->unlock();
			return;
		}
		vector<ffmap::iterator>* fmMapSequence=
			fp->getFeaturedMember(FM_MAP_SEQUENCE).m_pvpsMapSequence;
		if (fmMapSequence) {
			auto vi= std::find(fmMapSequence->begin(), fmMapSequence->end(), it);
			if (vi!=fmMapSequence->end()) {
				fmMapSequence->erase(vi);
			}
		}
		delete it->second;
		fp->val.pairs->erase(it);
		--size;
		fp->unlock();
	}
}

void Txj_::erase (int index) {
	if (isType(ARRAY)) {
		lock();
		if (index<size && (*val.array)[index]) {
			delete (*val.array)[index];
			(*val.array)[index]= NULL;
		}
		unlock();
	}
}

uint Txj_::erase (uint start, uint end) {
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

void Txj_::erase (Txj_* value) {
	if (isType(OBJ)||isType(ORDERED_OBJ)) {
		lock();
		ffmap::iterator i= val.pairs->begin();
		FeaturedMember fmMapSequence= getFeaturedMember(FM_MAP_SEQUENCE);
		while (i!=val.pairs->end()) {
			if (i->second==value) {
				if (fmMapSequence.m_pvpsMapSequence) {
					auto e= remove(fmMapSequence.m_pvpsMapSequence->begin(),
						fmMapSequence.m_pvpsMapSequence->end(), i);
					fmMapSequence.m_pvpsMapSequence->erase(e,
						fmMapSequence.m_pvpsMapSequence->end());
				}
				delete i->second;
				val.pairs->erase(i);
				--size;
				break;
			}
			++i;
		}
		unlock();
	} else if (isType(ARRAY)) {
		lock();
		int i= 0;
		while (i<size) {
			if ((*val.array)[i]==value) {
				delete (*val.array)[i];
				(*val.array)[i]= NULL;
				break;
			}
			++i;
		}
		unlock();
	} else if (isType(SET_TYPE)) {
		lock();
		ffset::iterator it= val.setPtr->find(value);
		if (it!=val.setPtr->end()) {
			delete *it;
			val.setPtr->erase(it);
			--size;
		}
		unlock();
	}
}

bool Txj_::inherit (Txj_& rObj, TxjPObj* pFPObj) {
	Txj_* pObj;
	if(!pFPObj->name)pFPObj= pFPObj->pObj;
	map<string, int>* m= nullptr;
	if (rObj.isLink()) {
		pObj= rObj.val.fptr;
		if (isType(ARRAY) && pObj->isType(ARRAY)) {
			int maxSize= size<pObj->size? size : pObj->size;
			m= new map<string, int>();
			int i= maxSize;
			while (i>0) {
				--i;
				(*m)[string((ccp)*(*pObj->val.array)[i])]= i;
			}
		} else {
			return false;
		}
	} else if (rObj.size==1) {
		//only links are allowed to be inherited
		//so parents should be declared first (:
		Txj_* arr= NULL;
		if (rObj.isType(ARRAY)) {
			if ((*rObj.val.array)[0] &&
				 (*rObj.val.array)[0]->isLink())
				arr= &rObj[0];
			else return false;
		} else if (rObj.isType(ORDERED_OBJ) || rObj.isType(OBJ)) {
			if ((*rObj.val.pairs)["*"]->isLink())
				arr= &rObj["*"];
			else return false;
		} else {
			return false;
		}
		m= new map<string, int>();
		int i= arr->size;
		while (i>0) {
			i--;
			(*m)[string((const char*) (*arr)[i])]= i;
		}
		if (isType(ARRAY)) {
			i= size;
			while (i>0) {
				i--;
				(*val.array)[i]->setEFlag(EXT_VIA_PARENT);
				FeaturedMember cFM;
				cFM.tabHead= m;
				(*val.array)[i]->insertFeaturedMember(cFM, FM_TABHEAD);
			}
		} else if (isType(OBJ) || isType(ORDERED_OBJ)) {
			// ffmap::iterator it= val.pairs->begin();
			// while (it!=val.pairs->end()) {
			// 	it->second->setEFlag(EXT_VIA_PARENT);
			// 	FeaturedMember cFM;
			// 	cFM.tabHead= m;
			// 	it->second->insertFeaturedMember(cFM, FM_TABHEAD);
			// 	it++;
			// }
			flErr(TXJ_MAIN, "Error parsing Txj_ at %d\n", i);
			return false;
		} else {
			return false;
		}
		//set "this" as child to the parent
		// Link& rLnParent= rObj.isType(ARRAY)?
		// 	*(*rObj.val.array)[0]->getFeaturedMember(FM_LINK).link
		// 	: *(*rObj.val.pairs)["*"]->getFeaturedMember(FM_LINK).link;
		// vector<const string*> path;
		// TxjPObj* pFPObjTemp= pFPObj;
		// bool bParentFound= false;
		// while (pFPObjTemp!=NULL) {
		// 	if (pFPObjTemp->value->isType(OBJ)) {
		// 		if (rLnParent.size() && pFPObjTemp->value->val.
		// 			 pairs->find(rLnParent[0])!=pFPObjTemp->
		// 			 value->val.pairs->end()) {
		// 			bParentFound= true;
		// 		}
		// 		path.push_back(pFPObjTemp->name);
		// 	} else if (pFPObjTemp->value->isType(ARRAY)) {
		// 		try {
		// 			path.push_back(pFPObjTemp->name);
		// 			if (pFPObjTemp->value->size>atoi(rLnParent[0].c_str())) {
		// 				bParentFound= true;
		// 			}
		// 		} catch (invalid_argument& ia) {
		// 			//flErr(TXJ_MAIN, "array member name is not a number");
		// 		}
		// 	}
		// 	if (bParentFound) {
		// 		Txj_* pParentRoot= pFPObjTemp->value;
		// 		int iParentLnIndexer= 0;
		// 		do {
		// 			if (pParentRoot->isType(OBJ)) {
		// 				pParentRoot= (*pParentRoot->val.pairs).
		// 					find(rLnParent[iParentLnIndexer++])
		// 					->second;
		// 			} else if (pParentRoot->isType(ARRAY)) {
		// 				try {
		// 					pParentRoot= (*pParentRoot->val.array)
		// 						[atoi(rLnParent[iParentLnIndexer++].c_str())];
		// 				} catch (Exception e) {
		// 					pParentRoot= NULL;
		// 				}
		// 			} else {
		// 				pParentRoot= NULL;
		// 			}
		// 			if (iParentLnIndexer<rLnParent.size())
		// 				pParentRoot= pFPObj->value->val.pairs->
		// 					at(rLnParent[iParentLnIndexer++]);
		// 		} while (pParentRoot && iParentLnIndexer<rLnParent.size());
		// 		if (pParentRoot) {
		// 			Txj_* pffLink= new Txj_();
		// 			pffLink->setType(LINK);
		// 			pffLink->val.fptr= this;
		// 			FeaturedMember cFM;
		// 			Link* pLnChild= new Link();
		// 			for (int i= path.size()-1; i>=0; i--) {
		// 				pLnChild->push_back(*path[i]);
		// 			}
		// 			cFM.link= pLnChild;
		// 			pffLink->insertFeaturedMember(cFM, FM_LINK);
		// 			if (!pParentRoot->isEFlagSet(HAS_CHILDREN)) {
		// 				pParentRoot->setEFlag(HAS_CHILDREN);
		// 				FeaturedMember fmChildren;
		// 				fmChildren.m_pvChildren= new vector<Txj_*>();
		// 				pParentRoot->insertFeaturedMember(
		// 					fmChildren, FM_CHILDREN);
		// 			}
		// 			vector<Txj_*>* pvfChildren= pParentRoot->
		// 				getFeaturedMember(FM_CHILDREN).m_pvChildren;
		// 			pvfChildren->push_back(pffLink);
		// 			break;
		// 		} else {
		// 			pFPObjTemp= pFPObjTemp->pObj;
		// 		}
		// 	} else {
		// 		pFPObjTemp= pFPObjTemp->pObj;
		// 	}
		// }
	} else if (rObj.size>1) {
		flErr(TXJ_MAIN, "Error parsing Txj_ in inherit.");
		return false;
	}
	FeaturedMember cFM;
	cFM.m_pParent= &rObj;
	setEFlag(EXTENDED);
	insertFeaturedMember(cFM, FM_PARENT);
	cFM.tabHead= m;
	insertFeaturedMember(cFM, FM_TABHEAD);
	return true;
}

Txj_::Iterator Txj_::begin () {
	Txj_* fp= this;
	if (isLink()) {
		return val.fptr->begin();
	}
	return Iterator(*fp);
}

Txj_::Iterator Txj_::end () {
	Txj_* fp= this;
	if (isLink()) {
		return val.fptr->end();
	}
	return Iterator(*fp, true);
}

Txj_::Iterator::Iterator () {
	type= NUL;
	m_uContainerPs.m_pMap=NULL;
}

Txj_::Iterator::Iterator (const Iterator& orig) {
	copy(orig);
}

Txj_::Iterator::Iterator (const Txj_& orig, bool end) {
	init(orig, end);
}

Txj_::Iterator::Iterator (ffmap::iterator pi) {
	ui.pi= pi;
	type= OBJ;
}

Txj_::Iterator::Iterator (vector<Txj_*>::iterator ai) {
	ui.ai= ai;
	type= ARRAY;
}

Txj_::Iterator::Iterator (
	vector<ffmap::iterator >::iterator pai, vector<ffmap::iterator>* pMapItVec
) {
	this->ui.pai= pai;
	type= OBJ;
	m_uContainerPs.m_pMapVector= pMapItVec;
}

Txj_::Iterator::~Iterator () {

}

void Txj_::Iterator::copy (const Iterator& i) {
	type= i.type;
	ui= i.ui;
	m_uContainerPs= i.m_uContainerPs;
}

void Txj_::Iterator::init (const Txj_& orig, bool end) {
	switch (orig.getType()) {
	case ARRAY: {
		type= ARRAY;
		ui.ai= end? orig.val.array->end() : orig.val.array->begin();
		m_uContainerPs.m_pVector= orig.val.array;
		break;
	}
	case ORDERED_OBJ:
	case OBJ: {
		FeaturedMember fm= orig.getFeaturedMember(FM_MAP_SEQUENCE);
		type= OBJ;
		if (fm.m_pvpsMapSequence!=NULL) {
			if (end) {
				ui.pai= fm.m_pvpsMapSequence->end();
			} else {
			  	ui.pai= fm.m_pvpsMapSequence->begin();
				while (ui.pai!=fm.m_pvpsMapSequence->end() &&
						 (*ui.pai)->first[0]=='#')
					++ui.pai; 
			}
			type= ORDERED_OBJ;
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

Txj_::Iterator& Txj_::Iterator::operator = (const Iterator& i) {
	copy(i);
	return *this;
}

string Txj_::Iterator::getIndex () {
	switch (type) {
	case ORDERED_OBJ:
		return (*ui.pai)->first;
	case OBJ:
		return ui.pi->first;
	default:
		flErr(
			TXJ_MAIN, "Index from non object type container is being retrieved."
		);
		return string ();
	}
}

int Txj_::Iterator::getIndex (const Txj_& rCurArray) {
	if (type==ARRAY) {
		return ui.ai-rCurArray.val.array->begin();
	} else {
		flDbg(TXJ_MAIN,
				"Index from non array type container is being retrieved.");
		return 0;
	}
}

Txj_& Txj_::Iterator::operator * () {
	Txj_* ptr= operator->();
	if (!ptr) return *nullTxj;
	return *ptr;
}

Txj_* Txj_::Iterator::operator -> () {
	switch (type) {
	case OBJ: return ui.pi->second;
	case ORDERED_OBJ: return (*ui.pai)->second;
	case ARRAY: return *ui.ai;
	case SET_TYPE: return *ui.si;
	}
	return nullptr;
}

Txj_::Iterator& Txj_::Iterator::operator ++ () {
	if (type==OBJ) {
		++ui.pi;
		while (ui.pi!=m_uContainerPs.m_pMap->end() &&
				 ui.pi->first[0]=='#') {
			++ui.pi;
		}
	} else if (type==ORDERED_OBJ) {
		++ui.pai;
		while (ui.pai!=m_uContainerPs.m_pMapVector->end() &&
				 (*ui.pai)->first[0]=='#') {
			++ui.pai;
		}
	} else if (type==ARRAY) {
		++ui.ai;
	} else if (type==SET_TYPE) {
		++ui.si;
	}
	return *this;
}

Txj_::Iterator Txj_::Iterator::operator ++ (int) {
	Txj_::Iterator tmp(*this);
	operator ++ ();
	return tmp;
}

Txj_::Iterator& Txj_::Iterator::operator -- () {
	if (type==OBJ) {
		ui.pi--;
		while (ui.pi->first[0]=='#') {
			--ui.pi;
		}
	} else if (type==ORDERED_OBJ) {
		--ui.pai;
		while ((*ui.pai)->first[0]=='#') {
			--ui.pai;
		}
	} else if (type==ARRAY) {
		--ui.ai;
	} else if (type==SET_TYPE) {
		--ui.si;
	}
	return *this;
}

Txj_::Iterator Txj_::Iterator::operator -- (int) {
	Txj_::Iterator tmp(*this);
	operator--();

	return tmp;
}

bool Txj_::Iterator::operator == (const Iterator& i) {
	if (type==i.type) {
		switch (type) {
		case ORDERED_OBJ:
			return ui.pai==i.ui.pai;
		case ARRAY:
			return ui.ai==i.ui.ai;
		case OBJ:
			return ui.pi==i.ui.pi;
		case SET_TYPE:
			return ui.si==i.ui.si;
		case NUL:
			return true;
		}
	}
	return false;
}

bool Txj_::Iterator::operator != (const Iterator& i) {
	bool res= operator==(i);
	return !res;
}

Txj_::Iterator::operator const char* () {
	switch (type) {
	case ORDERED_OBJ: return (*ui.pai)->first.c_str();
	case OBJ: return ui.pi->first.c_str();
	}
	return nullptr;
}

Txj_::Iterator Txj_::find (const string& key) {
	switch (getType()) {
		case ORDERED_OBJ: {
			const char lc= key.back();
			ffmap::iterator itMap;
			if (lc=='*') {
				string lk= key.substr(0,key.length()-1);
				itMap= val.pairs->lower_bound(lk);
				return Iterator(itMap);
			}
			itMap= val.pairs->find(key);
			FeaturedMember fm= getFeaturedMember(FM_MAP_SEQUENCE);
			if (fm.m_pvpsMapSequence) {
				vector<ffmap::iterator>::iterator itVecMap=
					fm.m_pvpsMapSequence->begin();
				vector<ffmap::iterator>::iterator itVecMapEnd=
					fm.m_pvpsMapSequence->end();
				while (itVecMap!=itVecMapEnd) {
					if (*itVecMap==itMap) {
						return Iterator(itVecMap, fm.m_pvpsMapSequence);
					}
					++itVecMap;
				}
				return Iterator(itVecMap, fm.m_pvpsMapSequence);
			} else {
				return Iterator(itMap);
			}
			break;
		}
		case OBJ: {
			const char lc= key.back();
			ffmap::iterator itMap;
			if (lc=='*') {
				string lk= key.substr(0,key.length()-1);
				itMap= val.pairs->lower_bound(lk);
				return Iterator(itMap);
			}
			itMap= val.pairs->find(key);
			return Iterator(itMap);
		}
		case ARRAY: {
			uint ikey= atoi(key.c_str());
			vector<Txj_*>::iterator itVec;
			if (ikey==0 && key!="0") {
				itVec= val.array->end();
			} else {
				itVec= val.array->begin()+ikey;
			}
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

ostream& operator << (ostream& out, const Txj_& f) {
	out << f.prettyString();
	return out;
}

bool operator < (const Txj_& lhs, const Txj_& rhs) {
	flDbg(TXJ_L2, "< operator");
	if (!lhs.isType(rhs.getType())) {
		if (lhs.isType(Txj_::LINK)) {
			return lhs.val.fptr<&rhs;
		} else if (rhs.isType(Txj_::LINK)) {
			return rhs.val.fptr<&lhs;
		}
		return lhs.getType()<rhs.getType();
	} else {
		switch(lhs.getType()) {
			case Txj_::NUMBER:
				return lhs.val.number<rhs.val.number;
			case Txj_::TIME:
				return *lhs.val.m_pFerryTimeStamp<*rhs.val.m_pFerryTimeStamp;
			case Txj_::STRING:
				return strcmp(lhs.val.str, rhs.val.str)<0;
			default:
				return (void*)lhs.val.fptr<(void*)rhs.val.fptr;
		}
	}
	return (const void*)&lhs<(const void*)&rhs;
}

bool operator == (const Txj_& lhs, const Txj_& rhs) {
	flDbg(TXJ_MAIN, "==operator");
	if (!lhs.isType(rhs.getType())) {
		if (lhs.isType(Txj_::LINK)) {
			return lhs.val.fptr==&rhs;
		} else if (rhs.isType(Txj_::LINK)) {
			return rhs.val.fptr==&lhs;
		}
		return false;
	}
	switch(lhs.getType()) {
		case Txj_::NUMBER:
			return lhs.val.number==rhs.val.number;
		case Txj_::TIME:
			return *lhs.val.m_pFerryTimeStamp==*rhs.val.m_pFerryTimeStamp;
		case Txj_::STRING:
		case Txj_::XML:
			if (*lhs.val.str==*rhs.val.str)
				return true;
			else
				return false;
		case Txj_::ORDERED_OBJ:
		case Txj_::OBJ: {
			if (lhs.size!=rhs.size)
				return false;
			ffmap::iterator it= lhs.val.pairs->begin();
			while (it!=lhs.val.pairs->end()) {
				ffmap::iterator rit= rhs.val.pairs->find(it->first);
				if (rit==rhs.val.pairs->end())
					return false;
				if (*rit->second!=*it->second)
					return false;
				++it;
			}
			return true;
		}
		case Txj_::SET_TYPE: {
			if (lhs.size!=rhs.size)
				return false;
			ffset::iterator lit= lhs.val.setPtr->begin();
			ffset::iterator rit= rhs.val.setPtr->begin();
			while (lit!=lhs.val.setPtr->end()) {
				if (**lit!=**rit)
					return false;
				++rit;++lit;
			}
			return true;
		}
		case Txj_::ARRAY:
			if (lhs.size!=rhs.size)
				return false;
			for (int i=0; i<lhs.size;++i)
				if (*(*rhs.val.array)[i]!=*(*lhs.val.array)[i])
					return false;
			return true;
		case Txj_::LINK:
		case Txj_::DLINK:
			return *lhs.val.fptr==*rhs.val.fptr;
		default:
			return (void*)lhs.val.fptr==(void*)rhs.val.fptr;
	}

	return (const void*)&lhs<(const void*)&rhs;
}

Txj_* Txj_::MarkAsUpdatable(string& link, const Txj_& rParent) {
	if (rParent.isType(OBJ) || rParent.isType(ARRAY)) {
		Txj_* pOrigParent= const_cast<Txj_*> (&rParent);
		Txj_* pParent= pOrigParent;
		vector<string> prop;
		explode(".", link, prop);
		for (int i= 0; i<prop.size(); ++i) {
			if (pParent->isType(OBJ) && pParent->val.pairs->find(prop[i])==
				 pParent->val.pairs->end()) {
				return NULL;
			} else if (pParent->isType(ARRAY) && atoi(prop[i].c_str())>=pParent->size) {
				return NULL;
			}
			pParent= &((*pParent)[prop[i]]);
		}
		pParent= pOrigParent;
		for (int i= 0; i<prop.size(); ++i) {
			TxjIterator UpdatablePair;
			if (pParent->isType(OBJ)) {
				UpdatablePair.m_itMap= pParent->val.pairs->find(prop[i]);
				sm_mUpdateObjs[pParent].insert(UpdatablePair);
			} else if (pParent->isType(ARRAY)) {
				UpdatablePair.m_uiIndex= atoi(prop[i].c_str());
				sm_mUpdateObjs[pParent].insert(
					UpdatablePair);
			}
			pParent= &(*pParent)[prop[i]];
		}
		if (sm_mUpdateObjs.find(pParent)==sm_mUpdateObjs.end()) {
			sm_mUpdateObjs[pParent];
		}
		pParent->setQType(UPDATE);
		return pParent;
	}
	return NULL;
}

Txj_* Txj_::UnMarkUpdatable (string& link, const Txj_& rParent) {
	Txj_* pParent= const_cast<Txj_*> (&rParent);
	if (rParent.isType(OBJ) || rParent.isType(ARRAY)) {
		map<Txj_*, Txj_*> rOrigParent;
		vector<string> prop;
		explode(".", link, prop);
		for (int i= 0; i<prop.size(); ++i) {
			if (pParent->isType(OBJ) && pParent->val.pairs->find(prop[i])==
				 pParent->val.pairs->end()) {
				return NULL;
			} else if (pParent->isType(ARRAY) && atoi(prop[i].c_str()) >=
						  pParent->size) {
				return NULL;
			}
			rOrigParent[(*pParent)[prop[i]]]= pParent;
			pParent= (*pParent)[prop[i]];
		}
		pParent->setQType(NONE);
		if (sm_mUpdateObjs.find(pParent)!=sm_mUpdateObjs.end() &&
			 sm_mUpdateObjs[pParent].size()==0) {
			string sObjName;
			for (int i= prop.size()-1; i>=0; ++i) {
				if (pParent->isType(OBJ))
					sm_mUpdateObjs[pParent].erase(pParent->val.pairs->
															find(sObjName));
				else if (pParent->isType(ARRAY) && sObjName.length()>0)
					sm_mUpdateObjs[pParent].erase(atoi(sObjName.c_str()));
				if (rParent.getQType()==UPDATE) {
					return pParent;
				}
				if (sm_mUpdateObjs[pParent].size()==0) {
					sm_mUpdateObjs.erase(pParent);
					sObjName= prop[i];
				} else {
					return pParent;
				}
				pParent= rOrigParent[pParent];
			}
			if (pParent->getQType()==UPDATE) {
				return pParent;
			}
			sm_mUpdateObjs.erase(pParent);
			return pParent;
		}
	}
	return NULL;
}

Txj_::LinkNRef Txj_::GetLinkString (TxjPObj* pObj) {
	LinkNRef lnr;
	while (pObj!=NULL) {
		lnr.m_sLink= *pObj->name+"."+lnr.m_sLink;
		lnr.m_pRef= pObj->value;
		pObj= pObj->pObj;
	}
	return lnr;
}

int Txj_::save (
	bool json, bool printComments, unsigned int indent,
	TxjPrettyPrintPObj* pObj, bool printFilePath, bool save
) const {
	string sOut;
	if (json)
		stringify(sOut, json);
	else
		prettyString(sOut, json, printComments, 0, pObj, false, save);
	if (isEFlagSet((E_FLAGS)(FILE|CASTFILE))) {
		const char* fn= getFeaturedMember(FM_FILE).m_sFileName;
		ofstream ofs(fn, ios::out|ios::trunc);
		if (ofs.is_open()) {
			ofs<< sOut;
			ofs.close();
			setEFlag(FILE);
			return sOut.length();
		} else {
			flWrn(TXJ_MAIN, "couldn't create %s", fn);
		}
		return -2;
	}
	return -1;
}

bool FFPtrCmp::operator() (const Txj_* a, const Txj_* b) const {
	// pointer comparison: comparing *a<*b goes through Txj_'s
	// `operator int()` conversion, which compares the raw union bytes
	// (val.number slot) as ints - two different objects with the same
	// union bytes (e.g. any fresh proxy, which is 0, and a stored 0/false)
	// were treated as equal, so set insert "deduped" them away and
	// operator= deleted the live proxy
	return a<b;
}
