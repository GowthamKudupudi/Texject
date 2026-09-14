/* 
 * Gowtham Kudupudi 29/09/2013
 * MIT License
 */

#ifndef Txj_H
#define Txj_H

#define MAX_ORDERED_MEMBERS 1000
#define MAX_MEM_ITER_UPDATE 100

#include "config.h"
#include <logger.h>
#include <ferrybase/FerryTimeStamp.h>
#include <ferrybase/mystdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <exception>
#include <list>
#include <set>
#include <stdint.h>
#include <cstring>
#include <shared_mutex>

typedef unsigned int uint;

using namespace std;

enum txj_log_level_ {
	TXJ_MAIN= 1 << 0,
	TXJ_L2=   1 << 1
};
class Txj_;
struct FFPtrCmp {
	bool operator() (const Txj_* a, const Txj_* b) const;
};
typedef set<Txj_*, FFPtrCmp> ffset;
typedef map<string, Txj_*> ffmap;
typedef vector<Txj_*> ffvec;
typedef const char* ccp;

class DLLExport Txj_ {
public:
	
	enum OBJ_TYPE : uint8_t {
		UNDEFINED,
		BOOL,
		BINARY,
		NUMBER,
		TIME,
		STRING,
		XML,
		SET_TYPE,
		NEW_SET_MEMBER,
		ARRAY,
		OBJ,
		ORDERED_OBJ,
		LINK,
		DLINK, // Direct link
		VPTR,
		NUL // string parsed returns at least NUL Txj_ if no exception
	};
	
	enum QUERY_TYPE : uint32_t {
		/**
		 * To clear query type
		 */
		NONE     = 0 << 8,
      QUERY    = 1 << 8,
      SET      = 2 << 8,
      DEL      = 3 << 8,
      UPDATE   = 4 << 8,
      NQUERY   = 5 << 8,
	};
	
	enum E_FLAGS : uint32_t {
		ENONE						= 0,

		B64ENCODE				= 1<<16, //ObjectsNArrayNStr
		B64ENCODE_CHILDREN	= 1<<17,
		B64ENCODE_STOP			= 1<<18,

		EXTENDED					= 1<<19, //ARRAY N OBJ //FM
		LONG_LAST_LN			= 1<<19, //STRING//linkResolvSerial //NO_FM
		
		PRECISION				= 1<<20, //NUMBER //FM
		EXT_VIA_PARENT			= 1<<20, //ARRAY N OBJ //FM
		ONE_SHORT_LAST_LN		= 1<<20, //STRING //NO_FM
		
		HAS_CHILDREN			= 1<<21, //ARRAY N OBJ //FM
		STRING_INIT				= 1<<21, //STRING //FM
		
		FILE						= 1<<22, //FILE //FM Any
		CASTFILE					= 1<<23
	};
	
	enum COPY_FLAGS : uint32_t {
		COPY_NONE      = 0,
      COPY_QUERIES   = 1<<0,
      COPY_EFLAGS    = 1<<1,
      COPY_SHALLOW   = 1<<2,
      COPY_ALL       = 1 | 1<<1
	};
	
	enum FeaturedMemType : uint32_t {
		FM_TABHEAD           = 1,
      FM_PRECISION         = 1,
      FM_WIDTH             = 1,
      FM_LINK              = 2,
      FM_PARENT            = 3,
      FM_CHILDREN          = 4,
      FM_MAP_SEQUENCE      = 5,
      FM_MULTI_LN          = 6,
      FM_UPDATE_TIMESTAMP  = 7,
      FM_FILE              = 8
	};
	
	class Exception : exception {
	public:
		Exception(string e) : identifier(e)
		{}
		const char* what() const throw () {
			return this->identifier.c_str();
		}
		~Exception() throw ()
		{}
	private:
		string identifier;
	};
	
	class Iterator {
	public:
		Iterator ();
		Iterator (const Iterator& orig);
		Iterator (const Txj_& orig, bool end= false);
		Iterator (map<string, Txj_*>::iterator pi);
		Iterator (vector<Txj_*>::iterator ai);
		Iterator (vector<ffmap::iterator>::iterator pai,
		          vector<ffmap::iterator>* pMapItVec);
		virtual     ~Iterator ();
      void        init (const Txj_& orig, bool end= false);
      Iterator&   operator ++ ();
      Iterator    operator ++ (int);
      Iterator&   operator -- ();
      Iterator    operator -- (int);
      Iterator&   operator = (const Iterator& i);
      Iterator&   operator + (int i);
      bool        operator == (const Iterator& i);
      bool        operator != (const Iterator& i);
      Txj_*        operator -> ();
      Txj_&        operator * ();
      operator    const char* ();
		
		/**
		 * Should be only used on OBJ type iterators
		 * @return name in the name-value pair of the iterator of the OBJ
		 */
		string getIndex ();
		/**
		 * Should be only use on ARRAY type iterators
		 * @param rCurrArray should be the Txj_ Object of the iterator
		 * @return index of the iterator of the ARRAY
		 */
		int getIndex (const Txj_& rCurrArray);
		
	private:
		uint8_t  type;
      void     copy (const Iterator& i);
		
		union IteratorUnion {
			ffmap::iterator                    pi;
         vector<Txj_*>::iterator             ai;
         ffset::iterator                    si;
         vector<ffmap::iterator>::iterator pai;
			
			IteratorUnion () {
				memset(this, 0, sizeof (IteratorUnion));
			}
			IteratorUnion (const IteratorUnion& rFIt) {
				memcpy(this, &rFIt, sizeof (IteratorUnion));
			}
			IteratorUnion (IteratorUnion&& rFIt) {
				memcpy(this, &rFIt, sizeof (IteratorUnion));
			}
			~IteratorUnion ()
			{}
			IteratorUnion (
				const vector<map<string, Txj_*>::iterator>::iterator& itMapVector
			) {
				pai= itMapVector;
			}
			IteratorUnion (const vector<Txj_*>::iterator& itVec) {
				ai= itVec;
			}
			IteratorUnion (const map<string, Txj_*>::iterator& itMap) {
				pi= itMap;
			}
			IteratorUnion (const ffset::iterator& itSet) {
				si= itSet;
			}
			IteratorUnion& operator = (const IteratorUnion& rFIt) {
				memcpy(this, &rFIt, sizeof (IteratorUnion));
				return *this;
			}
			IteratorUnion& operator = (IteratorUnion&& rFIt) {
				memcpy(this, &rFIt, sizeof (IteratorUnion));
				return *this;
			}
			// friend bool operator < (
			// 	const IteratorUnion& lhs,
			// 	const IteratorUnion& rhs
			// ) {
			// 	return true;
			// }
		} ui;
		union ContainerPs {
			ffmap*                     m_pMap;
         ffvec*                     m_pVector;
         vector<ffmap::iterator>*   m_pMapVector;
         ffset*                     m_pSet;
		} m_uContainerPs;
	};
	
	struct FeaturedMemHook;
	typedef vector<string> Link;
	struct Blob_ {
		uint8_t* p;
		size_t s;
	};
	union FeaturedMember {
		Link* link;
		map<string, int>* tabHead;
		Txj_* m_pParent;
		/**
		 * used to store the number precision
		 */
		unsigned int precision= 0;
		/**
		 * used to store the width of the string
		 */
		unsigned int width;
		/**
		 * used to link another Featured member
		 */
		FeaturedMemHook* m_pFMH;
		/**
		 * used for multiline buffer while parsing
		 */
		string* m_psMultiLnBuffer;
		/**
		 * used to mark a multi line array during init
		 */
		bool m_bIsMultiLineArray;
		/**
		 * array of links of all children. these links must be deleted up on
		 * change.
		 */
		vector<Txj_*>* m_pvChildren;
		/**
		 * Its a vector of names in a map for the order
		 */
		vector<ffmap::iterator>* m_pvpsMapSequence;
		/**
		 * file name
		 */
		char* m_sFileName;
		
		FerryTimeStamp* m_pTimeStamp;
		
		FeaturedMember() : link {NULL}
		{}
	};
	
	struct FeaturedMemHook {
		FeaturedMemHook() {
			m_uFM.m_pFMH= NULL;
			m_pFMH.m_pFMH= NULL;
		}
		FeaturedMember m_uFM;
		FeaturedMember m_pFMH;
	};
	
	struct TxjExt {
		Txj_* base= NULL;
	};
	struct SymlinkTrail {
		Link* l= nullptr;
		Txj_* ln= nullptr;
	};
	struct TxjPObj {
		const string* name= nullptr;
		Txj_* value= NULL;
		TxjPObj* pObj= NULL;
		vector<ffmap::iterator>* m_pvpsMapSequence= nullptr;
		vector<SymlinkTrail> symTrVec;
	};
	
	struct TxjPrettyPrintPObj : public TxjPObj {
		bool m_bHeaded= false;
		/**
		 * Holds column widths for tabular members
		 */
		map<string, vector<int> >* m_msviClWidths= NULL;
		/**
		 * If this flag is set returns 1st line of string
		 */
		bool m_bGiveFirstLine= false;
	};
	
	union TxjIterator {
		map<string, Txj_*>::iterator m_itMap;
		uint m_uiIndex;
		
		TxjIterator () {
			memset(this, 0, sizeof (TxjIterator));
		}
		TxjIterator (const TxjIterator& rFIt) {
			memcpy(this, &rFIt, sizeof (TxjIterator));
		}
		TxjIterator (TxjIterator&& rFIt) {
			memcpy(this, &rFIt, sizeof (TxjIterator));
		}
		~TxjIterator ()
		{}
		TxjIterator (const map<string, Txj_*>::iterator& itMap) {
			m_itMap= itMap;
		}
		TxjIterator (const unsigned int uiIndex) {
			m_uiIndex= uiIndex;
		}
		TxjIterator& operator = (const TxjIterator& rFIt) {
			memcpy(this, &rFIt, sizeof (TxjIterator));
			return *this;
		}
		
		TxjIterator& operator = (TxjIterator&& rFIt) {
			memcpy(this, &rFIt, sizeof (TxjIterator));
			return *this;
		}
		
		friend bool operator < (
			const TxjIterator& lhs,
			const TxjIterator& rhs
		) {
			return true;
		}
	};
	
	struct LinkNRef {
		Txj_* m_pRef= 0;
		string m_sLink;
	};

	union FFValue {
		char* str;
		vector<Txj_*>* array;
		map<string, Txj_*>* pairs;
		set<Txj_*, FFPtrCmp>* setPtr;
		double number;
		bool boolean;
		Txj_* fptr;
		uint8_t*	vptr;
		FerryTimeStamp* m_pFerryTimeStamp;
		FFValue() : str {nullptr}
		{}
	} val;

	/**
	 * It holds the size of the Txj_ object. array size, object properties,
	 * string length. Do not change it!! Its made public only for reading
	 * convenience.
	 */
	unsigned int size= 0;
	
	/**
	 * creates an UNRECOGNIZED Txj_ object. Any Txj_ object can be
	 * assigned any other type of Txj_ object.
	 */
	Txj_ ();
	
	/**
	 * Copy constructor. Creates a copy of Txj_ object
	 * @param orig is the object one wants to create a copy
	 */
	Txj_ (
		const Txj_& orig, COPY_FLAGS cf= COPY_ALL,
		TxjPObj* pObj= NULL
	);
	
	/**
	 * Creates a Txj_ object from a Txj_ string.
	 * @param ffjson is the Txj_ string to be parsed.
	 * @param ci is the offset in Txj_ string to be considered. Its 0 by
	 * default.
	 */
	Txj_ (
		const string& ffjson, int* ci= NULL, int indent= 0,
		TxjPObj* pObj= NULL
	);
	void init (
		const string& ffjson, int* ci= NULL, int indent= 0,
		TxjPObj* pObj= NULL
	);
	
	/**
	 * Creates an empty Txj_ object of type @param t. It throws an Exception
	 * if @param t is UNRECOGNIZED or anything else other Txj_OBJ_TYPE
	 * @param t
	 */
	Txj_(OBJ_TYPE t);
	
	~Txj_();
	/**
	 * Emptys the Txj_ object. For example If you want delete objects in an
	 * array or an object, invoke it.
	 */
	void freeObj(bool bAssignment=false);
	
	static const						FeaturedMemType m_FM_LAST= FM_PARENT;
	static const char					OBJ_STR[15][15];
	static inline std::map<std::string, uint8_t> STR_OBJ= {
		{"", UNDEFINED},
		{"UNDEFINED", UNDEFINED},
		{"str", STRING},
		{"xml", XML},
		{"num", NUMBER},
		{"bul", BOOL},
		{"obj", OBJ},
		{"oob", ORDERED_OBJ},
		{"arr", ARRAY},
		{"tm", TIME},
		{"NUL", NUL}
	};
	static map<const Txj_*, shared_mutex> MtxMap;
	static shared_mutex MtxMapMtx;
	void lock (); void unlock ();
	void lockShared () const; void unlockShared () const;
	bool tryLock ();
	bool tryLockShared () const;
	static void prune ();
	static Txj_* MarkAsUpdatable(string& link, const Txj_& rParent);
	static Txj_* UnMarkUpdatable(string& link, const Txj_& rParent);
	
	void insertFeaturedMember (FeaturedMember& fms, FeaturedMemType fMT);
	FeaturedMember getFeaturedMember (FeaturedMemType fMT) const;
	void destroyAllFeaturedMembers (bool bExemptQueries= false);
	void nullFeaturedMember (FeaturedMemType fmt);
	void deleteFeaturedMember (FeaturedMemType fmt);
	
	/**
	 * If the object is of type @param t, it returns true else false.
	 * @param t : type to check
	 * @return true if type matched
	 */
	bool isType (OBJ_TYPE t) const;
	bool isLink () const;
	/**
	 * Sets type of the object to t
	 * @param t
	 */
	void setType (OBJ_TYPE t);
	
	OBJ_TYPE getType () const;
	bool isQType (QUERY_TYPE t) const;
	void setQType (QUERY_TYPE t);
	QUERY_TYPE getQType () const;
	bool isEFlagSet (E_FLAGS t) const;
	void setEFlag (E_FLAGS t) const;
	E_FLAGS getEFlags () const;
	void clearEFlag (E_FLAGS t);
	void setFMCount (uint32_t iFMCount);
	
	/**
	 * Removes leading and trailing white spaces; sapces and tabs from a string.
	 * @param s
	 */
	static void trimWhites (string& s);
	/**
	 * Removes leading and trailing quotes in a string.
	 * @param s
	 */
	static void trimQuotes (string& s);
	/**
	 * Trying to read an object property that doesn't exist creates the property
	 * with unrecognized object.
	 * @param f
	 */
	void trim ();
	/**
	 * Gives Txj_ object type of Txj_ string.
	 * @param ffjson is the Txj_ string.
	 * @return Txj_ object type.
	 */
	OBJ_TYPE objectType (string ffjson);
	/**
	 * Converts Txj_ object into Txj_ string.
	 * @return Txj_ string.
	 */
	string stringify (
		bool json= false, bool bGetQueryStr= false,
		TxjPObj* pObj= NULL, uint lnLvl= 0
	) const;
	void stringify (
		string& str, bool json= false, bool bGetQueryStr= false,
		TxjPObj* pObj= NULL, uint lnLvl= 0
	) const;
	
	#define GetQueryString(...) stringify(false,true,NULL);
	
	/**
	 * Converts Txj_ object into Txj_ pretty string that has indents where
	 * they needed
	 * @param indent : Dont bother about it! Its 0 by default which you need. If
	 * you insist, it prepends its value number of indents to the output. To get
	 * an idea on what I'm saying, jst try it with non zero positive value.
	 * @return A pretty string :)
	 */
	string prettyString (
		bool json= false, bool printComments= false,
		int indent= 0, TxjPrettyPrintPObj* pObj= NULL,
		bool printFilePath= false, bool save= false
	) const;
	void prettyString (
		string& ps, bool json= false, bool printComments= false,
		int indent= 0, TxjPrettyPrintPObj* pObj= NULL,
		bool printFilePath= false, bool save= false
	) const;
	/**
	 * Generates a query string which can be used to query a Txj_ tree. Query
	 * string is constructed based on SET, QUERY and DELETE marks on the Txj_
	 * objects.
	 * E.g.
	 * {animals:{horses:{count:?,colors:?}}} can be used to query horses count
	 * and colors available!
	 * @return Query string.
	 */
	string queryString();
	/**
	 * Generates an answer string for a query object from the Txj_ tree. Query
	 * object is the Txj_ object returned by Txj_(queryString).
	 * E.g.
	 * {animals:{horses:{count:35,colors:["white","black","brown"]}}}
	 * can be the answer string for
	 * {animals:{horses:{count:?,colors:?}}}
	 * query string.
	 * @param queryString
	 * @return Answer string
	 */
	Txj_* answerString (Txj_& queryObject);
	
	Txj_* answerObject (
		Txj_* queryObject, TxjPObj* pObj= nullptr,
		FerryTimeStamp lastUpdateTime= FerryTimeStamp(), Txj_* ao= nullptr
	);
	void erase (string name);
	void erase (int index);
	void erase (Txj_* value);
	uint erase (uint start, uint end);
	int save (
		bool json= false, bool printComments= true, unsigned int indent=0,
		TxjPrettyPrintPObj* pObj=NULL, bool printFilePath=true,
		bool save=true
	) const;
	Iterator begin ();
	Iterator end ();
	Iterator find (const string& key);
	void SelfTest ();
	Txj_& addLink (const Txj_& obj, string label);
	Txj_& addLink (const string&& objPath, const string&& linkPath);
	Txj_& operator [] (const char* prop);
	Txj_& operator [] (const string& prop);
	Txj_& operator [] (const int index);
	Txj_& operator [] (void);
	
	template <typename T>
	Txj_& operator= (T* t) {
		freeObj ();
		val.vptr= (uint8_t*)t;
		setType(VPTR);
		return *this;
	}
	/**
	 * returns null Txj_ object if invalid pointer. Deletes the object on
	 * deleting this Txj_ object if its the last reference.
	 * @param t pointer to any object
	 * @return Txj_ object of BINARY type
	 */
	template <typename T>
	Txj_& operator= (const T& t) {
		lock();
		if(isQType(UPDATE)){
			FeaturedMember fm=getFeaturedMember(FM_UPDATE_TIMESTAMP);
			fm.m_pTimeStamp->update();
		}
		freeObj ();
		flDbg(TXJ_L2, "size:%d", sizeof(T));
		size= sizeof (T);
		val.vptr= (uint8_t*)malloc(size);
		(T&)(*val.vptr)= t;
		setType (BINARY);
		unlock();
		return *this;
	}
	Txj_& operator = (const char* s);
	//Txj_& operator = (char* s);
	Txj_& operator = (Blob_ b);
	Txj_& operator = (const string& s);
	Txj_& operator = (const int& i);
	Txj_& operator = (const unsigned int& i);
	Txj_& operator = (const double& d);
	Txj_& operator = (const float& f);
	Txj_& operator = (const short& s);
	Txj_& operator = (const long& l);
	Txj_& operator = (const bool& b);
	Txj_& operator = (const Txj_& f);
	Txj_& operator = (Txj_* f);

	Txj_& operator * ();
	Txj_* operator -> ();
	
	template<typename T>
	operator T& () {
		flDbg(TXJ_MAIN, "size:%d", sizeof(T));
		if ((isType(BINARY) && size==sizeof(T)) || isType(VPTR)) {
			return *reinterpret_cast<T*> (val.vptr);
		} else {
			flErr(TXJ_MAIN, "Illegal cast! Type miss match");
			return *reinterpret_cast<T*>(val.vptr);
		}
	}
	operator const char* ();
	operator double ();
	operator float ();
	operator bool ();
	operator int ();
	operator unsigned int ();
	operator long ();
	void copy (
		const Txj_& orig, COPY_FLAGS cf= COPY_NONE,
		TxjPObj* pObj= NULL
	);
private:
	mutable uint32_t flags= 0;
	FeaturedMember	m_uFM;
	static int getIndent (const char* ffjson, int* ci, int indent);
	static void strObjMapInit ();
	static bool inline isWhiteSpace (char c);
	static bool inline isTerminatingChar (char c);
	static bool inline isInitializingChar (char c);
	static std::map<Txj_*, set<TxjIterator> > sm_mUpdateObjs;
	Txj_* returnNameIfDeclared (vector<string>& prop,
										TxjPObj* fpo= nullptr) const;
	bool inherit (Txj_& obj, TxjPObj* pFPObj);
	void ReadMultiLinesInContainers (
		const string& ffjson, int& i,
		TxjPObj& pObj
	);
	string ConstructMultiLineStringArray (
		vector<Txj_*>& vpfMulLnStrs,
		int indent, vector<int>& vClWidths) const;
	LinkNRef GetLinkString (TxjPObj* pObj);
};
static Txj_ nullTxj;
ostream& operator << (ostream& out, const Txj_& f);

bool operator < (const Txj_& lhs, const Txj_& rhs);

#endif
