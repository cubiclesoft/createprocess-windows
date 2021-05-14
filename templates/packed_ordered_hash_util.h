// Ordered hash map with integer and string keys in a memory compact format.  No detachable nodes but is CPU cache-friendly.
// (C) 2017 CubicleSoft.  All Rights Reserved.

// NOTE:  This file is intended to be included from 'packed_ordered_hash.h'.

// Implements a packed ordered hash that grows dynamically and uses integer and string keys.
template <class T>
#ifdef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
class PackedOrderedHashNoCopy
#else
class PackedOrderedHash
#endif
{
public:
	// Implements djb2 (DJBX33X).
	// WARNING:  This algorithm is weak security-wise!
	// https://www.youtube.com/watch?v=R2Cq3CLI6H8
	// https://www.youtube.com/watch?v=wGYj8fhhUVA
	// For much better security with a slight performance reduction, use the other constructor, which implements SipHash.
#ifdef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
	PackedOrderedHashNoCopy
#else
	PackedOrderedHash
#endif
		(size_t EstimatedSize = 8, std::uint64_t HashKey = 5381) : UseSipHash(false), Key1(HashKey), Key2(0),
			ArrayNodes(NULL), HashNodes(NULL), Mask(0), NumNodes(0), NextNodePos(0), NumUsed(0),
			CompactNumUsed50(0), CompactNextNodePos75(0), CompactNumUsed90(0), ResizeNumUsed40(0)
	{
		ResizeHash(EstimatedSize);
	}

	// Keys are securely hashed via SipHash-2-4.
	// Assumes good (CSPRNG generated) inputs for HashKey1 and HashKey2.
#ifdef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
	PackedOrderedHashNoCopy
#else
	PackedOrderedHash
#endif
		(size_t EstimatedSize, std::uint64_t HashKey1, std::uint64_t HashKey2) : UseSipHash(true), Key1(HashKey1), Key2(HashKey2),
			ArrayNodes(NULL), HashNodes(NULL), Mask(0), NumNodes(0), NextNodePos(0), NumUsed(0),
			CompactNumUsed50(0), CompactNextNodePos75(0), CompactNumUsed90(0), ResizeNumUsed40(0)
	{
		ResizeHash(EstimatedSize);
	}

#ifdef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
	~PackedOrderedHashNoCopy()
#else
	~PackedOrderedHash()
#endif
	{
		PackedOrderedHashNode<T> *Node = ArrayNodes, *LastNode = ArrayNodes + NextNodePos;
		while (Node != LastNode)
		{
			if (Node->PrevHashIndex != 0xFFFFFFFF)
			{
				Node->Value.~T();

				if (Node->StrKey != NULL)  delete[] (Node->StrKey - sizeof(size_t));
			}

			Node++;
		}

		delete[] (char *)ArrayNodes;
	}

#ifdef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
	PackedOrderedHashNoCopy(const PackedOrderedHashNoCopy<T> &TempHash);
	PackedOrderedHashNoCopy<T> &operator=(const PackedOrderedHashNoCopy<T> &TempHash);
#else
	PackedOrderedHash(const PackedOrderedHash<T> &TempHash)
	{
		UseSipHash = TempHash.UseSipHash;
		Key1 = TempHash.Key1;
		Key2 = TempHash.Key2;

		ArrayNodes = NULL;
		NumNodes = 0;
		NumUsed = 0;

		InternalResizeHash(TempHash.NumNodes);
		NextNodePos = TempHash.NextNodePos;
		memcpy(HashNodes, TempHash.HashNodes, sizeof(std::uint32_t) * NumNodes);
		NumUsed = TempHash.NumUsed;

		PackedOrderedHashNode<T> *Node = TempHash.ArrayNodes, *Node2 = ArrayNodes, *LastNode = ArrayNodes + NextNodePos;

		while (Node != LastNode)
		{
			Node2->PrevHashIndex = Node->PrevHashIndex;
			Node2->NextHashIndex = Node->NextHashIndex;
			Node2->IntKey = Node->IntKey;

			if (Node->StrKey == NULL)  Node2->StrKey = NULL;
			else
			{
				size_t StrLen = Node->GetStrLen();
				char *Str = new char[StrLen + sizeof(size_t)];
				*((size_t *)Str) = StrLen;
				Str += sizeof(size_t);
				memcpy(Str, Node->StrKey, StrLen);
				Node2->StrKey = Str;
			}

			if (Node->PrevHashIndex != 0xFFFFFFFF)
			{
				new (&Node2->Value) T(Node->Value);
			}

			Node++;
			Node2++;
		}
	}

	PackedOrderedHash<T> &operator=(const PackedOrderedHash<T> &TempHash)
	{
		if (&TempHash != this)
		{
			UseSipHash = TempHash.UseSipHash;
			Key1 = TempHash.Key1;
			Key2 = TempHash.Key2;

			PackedOrderedHashNode<T> *Node = ArrayNodes, *Node2, *LastNode = ArrayNodes + NextNodePos;
			while (Node != LastNode)
			{
				if (Node->PrevHashIndex != 0xFFFFFFFF)
				{
					Node->Value.~T();

					if (Node->StrKey != NULL)  delete[] (Node->StrKey - sizeof(size_t));
				}

				Node++;
			}

			delete[] (char *)ArrayNodes;

			ArrayNodes = NULL;
			NumNodes = 0;
			NumUsed = 0;

			InternalResizeHash(TempHash.NumNodes);
			NextNodePos = TempHash.NextNodePos;
			memcpy(HashNodes, TempHash.HashNodes, sizeof(std::uint32_t) * NumNodes);
			NumUsed = TempHash.NumUsed;

			Node = TempHash.ArrayNodes;
			Node2 = ArrayNodes;
			LastNode = ArrayNodes + NextNodePos;

			while (Node != LastNode)
			{
				Node2->PrevHashIndex = Node->PrevHashIndex;
				Node2->NextHashIndex = Node->NextHashIndex;
				Node2->IntKey = Node->IntKey;

				if (Node->StrKey == NULL)  Node2->StrKey = NULL;
				else
				{
					size_t StrLen = Node->GetStrLen();
					char *Str = new char[StrLen + sizeof(size_t)];
					*((size_t *)Str) = StrLen;
					Str += sizeof(size_t);
					memcpy(Str, Node->StrKey, StrLen);
					Node2->StrKey = Str;
				}

				if (Node->PrevHashIndex != 0xFFFFFFFF)
				{
					new (&Node2->Value) T(Node->Value);
				}

				Node++;
				Node2++;
			}
		}

		return *this;
	}
#endif

	PackedOrderedHashNode<T> *Set(const std::int64_t IntKey)
	{
		size_t Pos;
		std::uint64_t HashKey = GetHashKey((const std::uint8_t *)&IntKey, sizeof(std::int64_t));
		PackedOrderedHashNode<T> *Node = InternalFind(IntKey, Pos, HashKey);

		if (Node != NULL)  return Node;

		// Create a new node.
		if (NextNodePos == NumNodes)  AutoResizeHash();

		Node = ArrayNodes + NextNodePos;
		new (&Node->Value) T;
		Pos = NextNodePos;
		NextNodePos++;

		// Attach the node to the start of the hash list.
		std::uint32_t HashPos = (std::uint32_t)HashKey & Mask;
		Node->PrevHashIndex = 0x80000000 | HashPos;
		Node->NextHashIndex = HashNodes[HashPos];
		if (Node->NextHashIndex != 0xFFFFFFFF)  ArrayNodes[Node->NextHashIndex].PrevHashIndex = (std::uint32_t)Pos;
		HashNodes[HashPos] = (std::uint32_t)Pos;

		Node->IntKey = IntKey;
		Node->StrKey = NULL;

		NumUsed++;

		return Node;
	}

	inline PackedOrderedHashNode<T> *Set(const std::int64_t IntKey, const T &Value)
	{
		PackedOrderedHashNode<T> *Node = Set(IntKey);
		Node->Value = Value;

		return Node;
	}

	PackedOrderedHashNode<T> *Set(const char *StrKey, const size_t StrLen)
	{
		size_t Pos;
		std::uint64_t HashKey = GetHashKey((const std::uint8_t *)StrKey, StrLen);
		PackedOrderedHashNode<T> *Node = InternalFind(StrKey, StrLen, Pos, HashKey);

		if (Node != NULL)  return Node;

		// Create a new node.
		if (NextNodePos == NumNodes)  AutoResizeHash();

		Node = ArrayNodes + NextNodePos;
		Pos = NextNodePos;
		new (&Node->Value) T;
		NextNodePos++;

		// Attach the node to the start of the hash list.
		std::uint32_t HashPos = (std::uint32_t)HashKey & Mask;
		Node->PrevHashIndex = 0x80000000 | HashPos;
		Node->NextHashIndex = HashNodes[HashPos];
		if (Node->NextHashIndex != 0xFFFFFFFF)  ArrayNodes[Node->NextHashIndex].PrevHashIndex = (std::uint32_t)Pos;
		HashNodes[HashPos] = (std::uint32_t)Pos;

		Node->IntKey = (std::int64_t)HashKey;

		char *Str = new char[StrLen + sizeof(size_t)];
		*((size_t *)Str) = StrLen;
		Str += sizeof(size_t);
		memcpy(Str, StrKey, StrLen);
		Node->StrKey = Str;

		NumUsed++;

		return Node;
	}

	inline PackedOrderedHashNode<T> *Set(const char *StrKey, const size_t StrLen, const T &Value)
	{
		PackedOrderedHashNode<T> *Node = Set(StrKey, StrLen);
		Node->Value = Value;

		return Node;
	}

	inline bool Unset(const std::int64_t IntKey)
	{
		return Unset(Find(IntKey));
	}

	inline bool Unset(const char *StrKey, const size_t StrLen)
	{
		return Unset(Find(StrKey, StrLen));
	}

	bool Unset(PackedOrderedHashNode<T> *Node)
	{
		if (Node == NULL || Node->PrevHashIndex == 0xFFFFFFFF)  return false;

		// Detach the node from the hash list.
		if (Node->NextHashIndex != 0xFFFFFFFF)  ArrayNodes[Node->NextHashIndex].PrevHashIndex = Node->PrevHashIndex;

		if (Node->PrevHashIndex & 0x80000000)  HashNodes[Node->PrevHashIndex & 0x7FFFFFFF] = Node->NextHashIndex;
		else  ArrayNodes[Node->PrevHashIndex].NextHashIndex = Node->NextHashIndex;

		// Cleanup.
		Node->PrevHashIndex = 0xFFFFFFFF;
		Node->Value.~T();
		if (Node->StrKey != NULL)
		{
			delete[] (Node->StrKey - sizeof(size_t));
			Node->StrKey = NULL;
		}

		NumUsed--;

		return true;
	}

	// Returns the node in the array by index.
	inline PackedOrderedHashNode<T> *Get(size_t Pos)
	{
		return (Pos >= NextNodePos || ArrayNodes[Pos].PrevHashIndex == 0xFFFFFFFF ? NULL : ArrayNodes + Pos);
	}

	// Gets the position of the node in the array.
	inline size_t GetPos(PackedOrderedHashNode<T> *Node) { return (size_t)(Node - ArrayNodes); }

	// Finds the node in the array via the hash.
	inline PackedOrderedHashNode<T> *Find(const std::int64_t IntKey)
	{
		size_t Pos;

		return Find(IntKey, Pos);
	}

	// Finds the node in the array via the hash.
	inline PackedOrderedHashNode<T> *Find(const std::int64_t IntKey, size_t &Pos)
	{
		return InternalFind(IntKey, Pos, GetHashKey((const std::uint8_t *)&IntKey, sizeof(std::int64_t)));
	}

	// Finds the node in the array via the hash.
	inline PackedOrderedHashNode<T> *Find(const char *StrKey, const size_t StrLen)
	{
		size_t Pos;

		return Find(StrKey, StrLen, Pos);
	}

	// Finds the node in the array via the hash.
	inline PackedOrderedHashNode<T> *Find(const char *StrKey, const size_t StrLen, size_t &Pos)
	{
		return InternalFind(StrKey, StrLen, Pos, GetHashKey((const std::uint8_t *)StrKey, StrLen));
	}

	// Iterates over the array, skipping unset nodes.  Initialize the input Pos to GetNextPos() to start at the beginning.
	inline PackedOrderedHashNode<T> *Next(size_t &Pos)
	{
		if (Pos >= NextNodePos)  Pos = 0;
		else  Pos++;

		for (; Pos < NextNodePos; Pos++)
		{
			if (ArrayNodes[Pos].PrevHashIndex != 0xFFFFFFFF)  return (ArrayNodes + Pos);
		}

		return NULL;
	}

	// Iterates over the array, skipping unset nodes.  Initialize the input Pos to GetNextPos() to start at the end.
	inline PackedOrderedHashNode<T> *Prev(size_t &Pos)
	{
		if (Pos > NextNodePos)  Pos = NextNodePos;

		while (Pos > 0)
		{
			Pos--;

			if (ArrayNodes[Pos].PrevHashIndex != 0xFFFFFFFF)  return (ArrayNodes + Pos);
		}

		Pos = NextNodePos;

		return NULL;
	}

	// Call before iterating over the array multiple times.  Then call Optimize() if this function returns true.
	inline bool ShouldOptimize()
	{
		return (NumUsed < CompactNumUsed50 && NextNodePos > CompactNextNodePos75);
	}

	// Compacts the array by moving elements to unset positions.
	bool Optimize()
	{
		if (NextNodePos == NumUsed)  return true;

		// Find the first unset position.
		PackedOrderedHashNode<T> *Node = ArrayNodes, *Node2, *LastNode = ArrayNodes + NextNodePos;
		while (Node != LastNode && Node->PrevHashIndex != 0xFFFFFFFF)  Node++;

		// Copy nodes.
		std::uint32_t x = (std::uint32_t)(Node - ArrayNodes);
		for (Node2 = Node + 1; Node2 != LastNode; Node2++)
		{
			if (Node2->PrevHashIndex != 0xFFFFFFFF)
			{
				// Raw copy node.
				memcpy(Node, Node2, sizeof(PackedOrderedHashNode<T>));

				// Update node hash indexes.
				if (Node->PrevHashIndex & 0x80000000)  HashNodes[Node->PrevHashIndex & 0x7FFFFFFF] = x;
				else  ArrayNodes[Node->PrevHashIndex].NextHashIndex = x;

				if (Node->NextHashIndex != 0xFFFFFFFF)  ArrayNodes[Node->NextHashIndex].PrevHashIndex = x;

				Node++;
				x++;
			}
		}

		// Cleanup extra nodes.
		for (; Node != LastNode; Node++)
		{
			Node->PrevHashIndex = 0xFFFFFFFF;
			Node->StrKey = NULL;
		}

		NextNodePos = NumUsed;

		return true;
	}

// Compact when resizing the hash.

	// Performs automatic resizing based on several rules:
	//   Compact instead of resizing the hash when NumUsed < 90%.
	//   Shrink when NumUsed < 40% full.
	//   Grows if NextNodePos is the same as NumNodes.
	bool AutoResizeHash()
	{
		if (NumUsed < CompactNumUsed90)  return Optimize();
		if (NumUsed < ResizeNumUsed40)  return InternalResizeHash(NumNodes >> 1);
		if (NextNodePos == NumNodes)  return InternalResizeHash(NumNodes << 1);

		return false;
	}

	bool ResizeHash(size_t NewHashSize)
	{
		size_t NewSize = 1;
		while (NewSize < NewHashSize)  NewSize <<= 1;

		return InternalResizeHash(NewSize);
	}

	inline size_t GetHashSize() { return NumNodes; }
	inline size_t GetNextPos() { return NextNodePos; }
	inline size_t GetSize() { return NumUsed; }

private:
	inline std::uint64_t GetHashKey(const std::uint8_t *Str, size_t Size) const
	{
		return (UseSipHash ? PackedOrderedHashUtil::GetSipHashKey(Str, Size, Key1, Key2, 2, 4) : (std::uint64_t)PackedOrderedHashUtil::GetDJBX33XHashKey(Str, Size, (size_t)Key1));
	}

	PackedOrderedHashNode<T> *InternalFind(const std::int64_t IntKey, size_t &Pos, std::uint64_t HashKey)
	{
		Pos = HashNodes[(std::uint32_t)HashKey & Mask];
		while (Pos != 0xFFFFFFFF)
		{
			if (ArrayNodes[Pos].IntKey == IntKey && ArrayNodes[Pos].StrKey == NULL)  return ArrayNodes + Pos;

			Pos = ArrayNodes[Pos].NextHashIndex;
		}

		return NULL;
	}

	PackedOrderedHashNode<T> *InternalFind(const char *StrKey, const size_t StrLen, size_t &Pos, std::uint64_t HashKey)
	{
		std::int64_t IntKey = (std::int64_t)HashKey;
		Pos = HashNodes[(std::uint32_t)HashKey & Mask];
		while (Pos != 0xFFFFFFFF)
		{
			if (ArrayNodes[Pos].IntKey == IntKey && ArrayNodes[Pos].StrKey != NULL && StrLen == ArrayNodes[Pos].GetStrLen() && !memcmp(StrKey, ArrayNodes[Pos].StrKey, StrLen))  return ArrayNodes + Pos;

			Pos = ArrayNodes[Pos].NextHashIndex;
		}

		return NULL;
	}

	bool InternalResizeHash(size_t NewHashSize)
	{
		while (NewHashSize < NumUsed)  NewHashSize <<= 1;
		if (NewHashSize == NumNodes || (NewHashSize < 512 && NewHashSize < NumNodes))  return false;

		PackedOrderedHashNode<T> *ArrayNodes2 = (PackedOrderedHashNode<T> *)(new char[sizeof(PackedOrderedHashNode<T>) * NewHashSize + sizeof(std::uint32_t) * NewHashSize]);
		std::uint32_t *HashNodes2 = (std::uint32_t *)(ArrayNodes2 + NewHashSize);

		memset(HashNodes2, 0xFF, sizeof(std::uint32_t) * NewHashSize);

		if (ArrayNodes != NULL)
		{
			std::uint64_t HashKey;
			std::uint32_t HashPos, Mask2, x = 0;
			PackedOrderedHashNode<T> *Node = ArrayNodes, *Node2 = ArrayNodes2, *LastNode = ArrayNodes + NextNodePos;
			Mask2 = (std::uint32_t)(NewHashSize - 1);
			while (Node != LastNode)
			{
				if (Node->PrevHashIndex != 0xFFFFFFFF)
				{
					// Raw copy node.
					memcpy(Node2, Node, sizeof(PackedOrderedHashNode<T>));

					HashKey = (Node2->StrKey != NULL ? (std::uint64_t)Node2->IntKey : GetHashKey((const std::uint8_t *)&Node2->IntKey, sizeof(std::int64_t)));

					// Attach the node to the start of the hash list.
					HashPos = (std::uint32_t)HashKey & Mask2;
					Node2->PrevHashIndex = 0x80000000 | HashPos;
					Node2->NextHashIndex = HashNodes2[HashPos];
					if (Node2->NextHashIndex != 0xFFFFFFFF)  ArrayNodes2[Node2->NextHashIndex].PrevHashIndex = x;
					HashNodes2[HashPos] = x;

					Node2++;
					x++;
				}

				Node++;
			}
		}

		ArrayNodes = ArrayNodes2;
		HashNodes = HashNodes2;
		NumNodes = NewHashSize;
		Mask = (std::uint32_t)(NumNodes - 1);
		NextNodePos = NumUsed;

		CompactNumUsed50 = (NumNodes >> 1);
		CompactNextNodePos75 = NumNodes - (NumNodes >> 3);
		CompactNumUsed90 = NumNodes - (NumNodes / 10);
		ResizeNumUsed40 = (size_t)((std::uint64_t)(NumNodes << 3) / 10);

		return true;
	}

	bool UseSipHash;
	std::uint64_t Key1, Key2;

	PackedOrderedHashNode<T> *ArrayNodes;
	std::uint32_t *HashNodes;

	std::uint32_t Mask;
	size_t NumNodes, NextNodePos, NumUsed;
	size_t CompactNumUsed50, CompactNextNodePos75;
	size_t CompactNumUsed90, ResizeNumUsed40;
};
