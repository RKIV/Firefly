#pragma once

#include <cstdint>
#include <bitset>
#include <vector>


extern uint32_t componentCounter;

typedef uint64_t EntityID;

constexpr uint32_t MAX_COMPONENTS = 32;
typedef std::bitset<MAX_COMPONENTS> ComponentMask;

constexpr uint32_t NUM_COMPONENTS_PER_CHUNK = 64;

constexpr uint32_t NUM_CHUNKS_PER_POOL = 8;
constexpr uint32_t MAX_ENTITIES = NUM_CHUNKS_PER_POOL * NUM_COMPONENTS_PER_CHUNK;

/**
 * @tparam T The class of the component to get the ID for
 * @return ID of the component class
 */
template <class T>
uint32_t GetComponentId()
{
	static uint32_t componentId = componentCounter++;
	return componentId;
}

struct Scene
{
	struct EntityDesc
	{
		EntityID id;
		ComponentMask mask;
	};

	EntityID CreateEntity();

	void DestroyEntity(EntityID id);

	template<typename T>
	T* GetOrAddComponent(EntityID id)
	{
		EntityIndex entityIdx = GetEntityIndex(id);

		if (mEntities[entityIdx].id != id)
		{
			return nullptr;
		}

		const uint32_t componentId = GetComponentId<T>();

		if(mComponentPools.size() <= componentId)
		{
			mComponentPools.resize(componentId + 1, nullptr);
		}
			
		if(mComponentPools[componentId] == nullptr)
		{
			mComponentPools[componentId] = new ComponentPool(sizeof(T));
		}

		mEntities[entityIdx].mask.set(componentId);

		return static_cast<T*>(mComponentPools[componentId]->GetOrCreateComponent(id));
	}

	template<typename T>
	void RemoveComponent(EntityID id)
	{
		if (mEntities[GetEntityIndex(id)].id != id)
		{
			return;
		}

		const int componentId = GetComponentId<T>();

		if (ComponentPool* pool = mComponentPools[componentId])
		{
			pool->FreeComponent(id);
		}
		
		mEntities[GetEntityIndex(id)].mask.reset(componentId);
	}

#ifndef NDEBUG
	void DebugPrintState() const;
#endif
	
private:

	typedef uint32_t EntityIndex;
	typedef uint32_t EntityVersion;

	static EntityID CreateEntityId(const EntityIndex index, const EntityVersion version)
	{
		return static_cast<EntityID>(index) << 32 | version;
	}

	static EntityIndex GetEntityIndex(const EntityID id)
	{
		return id >> 32;
	}

	static EntityVersion GetEntityVersion(const EntityID id)
	{
		return static_cast<EntityVersion>(id);
	}

	static bool IsEntityValid(EntityID id)
	{
		return (id >> 32) != static_cast<EntityIndex>(-1);
	}

	std::vector<EntityDesc> mEntities;
	std::vector<EntityIndex> mFreeEntities;

private:
	struct ComponentPoolChunk
	{
		ComponentPoolChunk() = default;

		ComponentPoolChunk(const ComponentPoolChunk&) = delete;
		ComponentPoolChunk& operator =(const ComponentPoolChunk&) = delete;

		ComponentPoolChunk(ComponentPoolChunk&& Other) noexcept;
		ComponentPoolChunk& operator=(ComponentPoolChunk&& Other) noexcept;

		explicit ComponentPoolChunk(size_t inComponentSize);

		~ComponentPoolChunk();

		/**
		 * Allocate a new component in this chunk
		 * Presumes that there is free space in the chunk
		 * @param id The id of the entity that will be associated with this component
		 * @return The index of the allocated component within the chunk
		 */
		uint32_t AllocateComponent(EntityID id);

		/**
		 * Free a previously allocated component
		 * @param index The index of the component within the chunk to free
		 */
		void FreeComponent(uint32_t index);

		[[nodiscard]] void* GetComponent(uint32_t index) const;

		[[nodiscard]] EntityID GetEntityId(uint32_t idx) const;
	
		[[nodiscard]] bool IsEmpty() const { return ~freeComponents == 0; }

		[[nodiscard]] bool IsFull() const { return freeComponents == 0; }

		[[nodiscard]] bool IsValid() const { return componentSize > 0 && pData != nullptr; }

		size_t componentSize = 0;
		std::bitset<NUM_COMPONENTS_PER_CHUNK> freeComponents{};
		uint8_t* pData = nullptr;
	};


	struct ComponentPool
	{
		explicit ComponentPool(size_t inComponentSize);

		void* GetOrCreateComponent(EntityID id);

		void FreeComponent(EntityID id);

#ifndef NDEBUG
		void DebugPrintState() const;
#endif

		ComponentPoolChunk chunks[NUM_CHUNKS_PER_POOL];
		uint32_t sparseMap[MAX_ENTITIES] = {};
		size_t componentSize = 0;
	};
	
	// TODO: Could probably turn this into a map
	std::vector<ComponentPool*> mComponentPools;
};

struct TestComponent1
{
	int32_t param1;
	int32_t param2;
};

struct TestComponent2
{
	int32_t param1;
	int32_t param2;
	int32_t param3;
};