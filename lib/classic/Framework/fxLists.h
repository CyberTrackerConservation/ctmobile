/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

#pragma once
#include "pch.h"
#include "fxTypes.h"

template<class T> class TList
{
protected:
    T *_list;

public:
    TList(): _list(0) 
    {
    }

    ~TList()
    {
        MEM_FREE(_list); 
    }

    T Get(UINT Index)
    { 
        if (Index >= GetCount())
        {
            Index--;
            Index++;
        }
        FX_ASSERT(Index < GetCount());
        return *(_list + Index);
    }

    T *GetPtr(UINT Index)
    { 
        FX_ASSERT(Index < GetCount());
        return _list + Index;
    }

    VOID Put(UINT Index, T Item)
    {
        memmove(_list + Index, &Item, sizeof(T));
    }

    UINT GetCount()
    { 
		if (_list)
		{
			FX_ASSERT(MEM_CHECK(_list));
			return MEM_SIZE(_list) / sizeof(T); 
		}
		else
		{
			return 0;
		}
    }

    VOID SetCount(UINT NewCount)
    {
		if (NewCount == 0)
		{
			MEM_FREE(_list);
			_list = NULL;
			return;
		}

		T *pTemp = (T *) MEM_ALLOC(NewCount * sizeof(T)); 
		memset(pTemp, 0, NewCount * sizeof(T));
    
		if (_list) 
		{
			memmove(pTemp, _list, min(GetCount(), NewCount) * sizeof(T)); 
			MEM_FREE(_list); 
		}
    
		_list = pTemp;
    }

    UINT Add(T Item)
    {
        UINT count = GetCount();
        SetCount(count + 1);

        Put(count, Item);

        return count;
    }

    UINT Insert(UINT Index, T Item)
    {
        UINT count = GetCount();

        SetCount(count + 1); 

        if (Index < count) 
        {
            memmove(_list + Index + 1, _list + Index, (count - Index) * sizeof(T));
        }

        Put(Index, Item);
        
        return Index;
    }

    VOID Delete(UINT Index)
    {
        UINT count = GetCount()-1;
        if (Index < count) 
        {
            memmove(_list + Index, _list + Index + 1, (count - Index) * sizeof(T));
        }
        SetCount(count);
    }

    UINT Remove(T Item)
    {
        INT result = IndexOf(Item);
        
        if (result >= 0) 
        {
            Delete(result);
        }
        
        return result;
    }

	VOID Push(T Item) 
	{
		Add(Item);
	}

	T Pop()
	{
		FX_ASSERT(GetCount() > 0);
        return Get(GetCount()-1);
	}

    VOID Clear()
    {
        SetCount(0);
    }

    VOID Exchange(UINT Index1, UINT Index2)
    {
        FX_ASSERT((Index1 >= 0) && (Index1 < GetCount()));
        FX_ASSERT((Index2 >= 0) && (Index2 < GetCount()));

        T *pTemp = (T *) MEM_ALLOC(sizeof(T));
        memmove(pTemp, _list + Index1, sizeof(T));
        memmove(_list + Index1, _list + Index2, sizeof(T));
        memmove(_list + Index2, pTemp, sizeof(T));

        MEM_FREE(pTemp);
    }

    VOID Move(UINT CurrentIndex, UINT NewIndex)
    {
        if (CurrentIndex != NewIndex) 
        {
            FX_ASSERT((NewIndex >=0) && (NewIndex < GetCount()));                

            T *pTemp = (T *) MEM_ALLOC(sizeof(T));
            memmove(pTemp, _list + CurrentIndex, sizeof(T));
            Delete(CurrentIndex);
            Insert(NewIndex, *pTemp);
            MEM_FREE(pTemp);
        }
    }

    BOOL Empty()
    {
        return GetCount() == 0;
    }

    T *First()
    {
        return Empty() ? NULL : GetPtr(0);
    }

    T *Last()
    {
        return Empty() ? NULL : GetPtr(GetCount() - 1);
    }

    INT IndexOf(T Item)
    {
        INT count = (INT)GetCount();
        INT result = 0;
        while ((result < count) && (memcmp(_list + result, &Item, sizeof(T)) != 0)) 
        {
            result++;
        }

        if (result == count) 
        {
            result = -1;
        }

        return result;
    }

    VOID *GetMemory()
    {
        return _list;
    }

    VOID *GetMemoryPtr()
    {
        return &_list;
    }

    VOID LoadFromStream(CfxStream &Stream)
    {
        UINT count;
        Stream.Read(&count, sizeof(count));
        SetCount(count);
        Stream.Read((VOID *)_list, sizeof(T) * count);
    }

    VOID SaveToStream(CfxStream &Stream)
    {
        UINT count = GetCount();
        Stream.Write(&count, sizeof(count));
        Stream.Write((VOID *)_list, sizeof(T) * count);
    }

    VOID Assign(TList<T> *pSource)
    {
        if (pSource->GetCount() == GetCount())
        {
            memcpy(_list, pSource->GetMemory(), GetCount() * sizeof(T));
        }
        else
        {
            CfxStream stream;
            pSource->SaveToStream(stream);
            stream.SetPosition(0);
            LoadFromStream(stream);
        }
    }
};

typedef TList<VOID *> CfxList;
typedef TList<XGUID> XGUIDLIST;
typedef TList<GUID> GUIDLIST;
typedef TList<FXFILE> FXFILELIST;

class XGUIDHASHMAP {
private:
    XGUIDLIST _table[256];
public:
    XGUIDHASHMAP(): _table()
    {
    }

    UINT GetKey(XGUID Value)
    {
        UINT v;
        if (sizeof(Value) == 4)
        {
            v = *(UINT *)&Value;
        }
        else
        {
            UINT *p = (UINT *)&Value;
            v = 0;
            for (UINT i=0; i < sizeof(Value) / sizeof(UINT); i++, p++)
            {
                v += *p;
            }
        }

        v = ((v >> 16) ^ v) * 0x45d9f3b;
        v = ((v >> 16) ^ v) * 0x45d9f3b;
        v = ((v >> 16) ^ v);

        return v;
    }

    VOID Insert(XGUID Value)
    {
        UINT index = GetKey(Value) % ARRAYSIZE(_table);
        _table[index].Add(Value);
    }

    BOOL Exists(XGUID Value)
    {
        UINT index = GetKey(Value) % ARRAYSIZE(_table);
        
        for (UINT i=0; i<_table[index].GetCount(); i++)
        {
            if (memcmp(&Value, _table[index].GetPtr(i), sizeof(XGUID)) == 0)
            {
                return TRUE;
            }
        }

        return FALSE;
    }
};
