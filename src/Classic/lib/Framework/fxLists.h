#pragma once
#include "pch.h"
#include "fxTypes.h"

template<class T> class TList
{
protected:
    T *_list = nullptr;

public:
    TList(): _list(0) 
    {
    }

    ~TList()
    {
        MEM_FREE(_list); 
    }

    T Get(int Index)
    { 
        if (Index >= GetCount())
        {
            Index--;
            Index++;
        }
        FX_ASSERT(Index < GetCount());
        return *(_list + Index);
    }

    T *GetPtr(int Index)
    { 
        FX_ASSERT(Index < GetCount());
        return _list + Index;
    }

    void Put(int Index, T Item)
    {
        FX_ASSERT(_list);
        memmove(_list + Index, &Item, sizeof(T));
    }

    int GetCount()
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

    void SetCount(int NewCount)
    {
		if (NewCount == 0)
		{
			MEM_FREE(_list);
			_list = nullptr;
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

    int Add(T Item)
    {
        int count = GetCount();
        SetCount(count + 1);

        Put(count, Item);

        return count;
    }

    int Insert(int Index, T Item)
    {
        int count = GetCount();

        SetCount(count + 1); 

        if (Index < count) 
        {
            memmove(_list + Index + 1, _list + Index, (count - Index) * sizeof(T));
        }

        Put(Index, Item);
        
        return Index;
    }

    void Delete(int Index)
    {
        int count = GetCount()-1;
        if (Index < count) 
        {
            memmove(_list + Index, _list + Index + 1, (count - Index) * sizeof(T));
        }
        SetCount(count);
    }

    int Remove(T Item)
    {
        int result = IndexOf(Item);
        
        if (result >= 0) 
        {
            Delete(result);
        }
        
        return result;
    }

	void Push(T Item) 
	{
		Add(Item);
	}

	T Pop()
	{
		FX_ASSERT(GetCount() > 0);
        return Get(GetCount()-1);
	}

    void Clear()
    {
        SetCount(0);
    }

    void Exchange(int Index1, int Index2)
    {
        FX_ASSERT((Index1 >= 0) && (Index1 < GetCount()));
        FX_ASSERT((Index2 >= 0) && (Index2 < GetCount()));

        T *pTemp = (T *) MEM_ALLOC(sizeof(T));
        memmove(pTemp, _list + Index1, sizeof(T));
        memmove(_list + Index1, _list + Index2, sizeof(T));
        memmove(_list + Index2, pTemp, sizeof(T));

        MEM_FREE(pTemp);
    }

    void Move(int CurrentIndex, int NewIndex)
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

    bool Empty()
    {
        return GetCount() == 0;
    }

    T *First()
    {
        return Empty() ? nullptr : GetPtr(0);
    }

    T *Last()
    {
        return Empty() ? nullptr : GetPtr(GetCount() - 1);
    }

    int IndexOf(T Item)
    {
        int count = (int)GetCount();
        int result = 0;
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

    void *GetMemory()
    {
        return _list;
    }

    void *GetMemoryPtr()
    {
        return &_list;
    }

    void LoadFromStream(CfxStream &Stream)
    {
        int count;
        Stream.Read(&count, sizeof(count));
        SetCount(count);
        Stream.Read((void *)_list, sizeof(T) * count);
    }

    void SaveToStream(CfxStream &Stream)
    {
        int count = GetCount();
        Stream.Write(&count, sizeof(count));
        Stream.Write((void *)_list, sizeof(T) * count);
    }

    void Assign(TList<T> *pSource)
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

typedef TList<void *> CfxList;
typedef TList<XGUID> XGUIDLIST;
typedef TList<GUID> GUIDLIST;
typedef TList<FXFILE> FXFILELIST;

class XGUIDHASHMAP {
private:
    XGUIDLIST _table[256] = {};
public:
    XGUIDHASHMAP(): _table()
    {
    }

    int GetKey(XGUID Value)
    {
        int v;
        if (sizeof(Value) == 4)
        {
            v = *(int *)&Value;
        }
        else
        {
            int *p = (int *)&Value;
            v = 0;
            for (int i=0; i < sizeof(Value) / sizeof(int); i++, p++)
            {
                v += *p;
            }
        }

        v = ((v >> 16) ^ v) * 0x45d9f3b;
        v = ((v >> 16) ^ v) * 0x45d9f3b;
        v = ((v >> 16) ^ v);

        return v;
    }

    void Insert(XGUID Value)
    {
        int index = GetKey(Value) % ARRAYSIZE(_table);
        _table[index].Add(Value);
    }

    bool Exists(XGUID Value)
    {
        int index = GetKey(Value) % ARRAYSIZE(_table);
        
        for (int i=0; i<_table[index].GetCount(); i++)
        {
            if (memcmp(&Value, _table[index].GetPtr(i), sizeof(XGUID)) == 0)
            {
                return true;
            }
        }

        return false;
    }
};
