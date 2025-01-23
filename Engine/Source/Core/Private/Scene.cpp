#include "Scene.h"

#include <iostream>
#include <cassert>
#include <format>

uint32_t componentCounter = 0;

EntityID Scene::CreateEntity()
{
    assert(!mFreeEntities.empty() || mEntities.size() < MAX_ENTITIES);
    if (!mFreeEntities.empty())
    {
        EntityIndex freeIndex = mFreeEntities.back();
        mFreeEntities.pop_back();
        return mEntities[freeIndex].id = CreateEntityId(freeIndex, GetEntityVersion(mEntities[freeIndex].id));
    }
    
    mEntities.push_back({CreateEntityId(static_cast<EntityIndex>(mEntities.size()), 0), ComponentMask()});
    return mEntities.back().id;
}

void Scene::DestroyEntity(EntityID id)
{
    if (mEntities[GetEntityIndex(id)].id != id)
    {
        return;
    }
    
    for (ComponentPool* pPool : mComponentPools)
    {
        if (pPool)
        {
            pPool->FreeComponent(id);
        }
    }
    
    mEntities[GetEntityIndex(id)].id = CreateEntityId(static_cast<EntityIndex>(-1), GetEntityVersion(id) + 1);
    mEntities[GetEntityIndex(id)].mask.reset();
    
    mFreeEntities.push_back(GetEntityIndex(id));
}

Scene::ComponentPoolChunk::ComponentPoolChunk(ComponentPoolChunk&& Other) noexcept
{
    componentSize = Other.componentSize;
    Other.componentSize = 0;
    freeComponents = Other.freeComponents;
    Other.freeComponents.reset();
    delete[] pData;
    pData = Other.pData;
    Other.pData = nullptr;
}

Scene::ComponentPoolChunk& Scene::ComponentPoolChunk::operator=(ComponentPoolChunk&& Other) noexcept
{
    componentSize = Other.componentSize;
    Other.componentSize = 0;
    freeComponents = Other.freeComponents;
    Other.freeComponents.reset();
    delete[] pData;
    pData = Other.pData;
    Other.pData = nullptr;
    return *this;
}

Scene::ComponentPoolChunk::ComponentPoolChunk(size_t inComponentSize)
{
    componentSize = inComponentSize;
    freeComponents.set();
    // This EntityID at the end will store the sparse array id
    pData = new uint8_t[(componentSize + sizeof(EntityID)) * NUM_COMPONENTS_PER_CHUNK];
}

Scene::ComponentPoolChunk::~ComponentPoolChunk()
{
    componentSize = 0;
    delete[] pData;
    pData = nullptr;
}

uint32_t Scene::ComponentPoolChunk::AllocateComponent(const EntityID id)
{
    assert(freeComponents.any());
    assert(IsValid());
		
    uint8_t firstFreeIndex = 0;
    while(!freeComponents.test(firstFreeIndex))
    {
        firstFreeIndex++;
    }
    freeComponents.reset(firstFreeIndex);
    EntityID* idPtr = reinterpret_cast<EntityID*>(pData + firstFreeIndex * (componentSize + sizeof(EntityID)) + componentSize);
    *idPtr = id; 
    return firstFreeIndex;
    return 0;
}

void Scene::ComponentPoolChunk::FreeComponent(const uint32_t index)
{
    assert(index < NUM_COMPONENTS_PER_CHUNK);
    assert(!freeComponents.test(index));
    assert(IsValid());

    freeComponents.set(index);
}

void* Scene::ComponentPoolChunk::GetComponent(const uint32_t index) const
{
    assert(index < NUM_COMPONENTS_PER_CHUNK);
    assert(!freeComponents.test(index));
    assert(IsValid());

    return pData + index * (componentSize + sizeof(EntityID));
}

EntityID Scene::ComponentPoolChunk::GetEntityId(const uint32_t idx) const
{
    assert(!freeComponents.test(idx));
    const EntityID* pId = reinterpret_cast<EntityID*>(pData + idx * (componentSize + sizeof(EntityID)) + componentSize);
    return *pId;
}

Scene::ComponentPool::ComponentPool(size_t inComponentSize)
{
    componentSize = inComponentSize;
}

void* Scene::ComponentPool::GetOrCreateComponent(const EntityID id)
{
    const EntityIndex entityIdx = GetEntityIndex(id);
    
    assert(entityIdx < MAX_ENTITIES);

    if(sparseMap[entityIdx] == 0)
    {
        for(uint32_t chunkIdx = 0; chunkIdx < NUM_CHUNKS_PER_POOL; ++chunkIdx)
        {
            if(!chunks[chunkIdx].IsValid())
            {
                chunks[chunkIdx] = ComponentPoolChunk(componentSize);
            }

            if(chunks[chunkIdx].IsValid() && !chunks[chunkIdx].IsFull())
            {
                const uint32_t innerIdx = chunks[chunkIdx].AllocateComponent(id);
                // 0 is our null value so we store the actual idx + 1
                sparseMap[entityIdx] = innerIdx + chunkIdx * NUM_COMPONENTS_PER_CHUNK + 1;
                return chunks[chunkIdx].GetComponent(innerIdx);
            }
        }

        // Second pass if we couldn't find any empty chunks then we try to allocate new ones
        for(uint32_t chunkIdx = 0; chunkIdx < NUM_CHUNKS_PER_POOL; ++chunkIdx)
        {
            if(!chunks[chunkIdx].IsValid())
            {
                new(&chunks[chunkIdx]) ComponentPoolChunk(componentSize);
                const uint32_t innerIdx = chunks[chunkIdx].AllocateComponent(id);
                sparseMap[entityIdx] = innerIdx + chunkIdx * NUM_COMPONENTS_PER_CHUNK + 1;
                return chunks[chunkIdx].GetComponent(innerIdx);
            }
        }

        // We ran out of chunk space?
        assert(false);
        return nullptr;
    }
    else
    {
        const uint32_t chunkIdx = (sparseMap[entityIdx] - 1) / NUM_COMPONENTS_PER_CHUNK;
        const uint32_t innerIdx = (sparseMap[entityIdx] - 1) % NUM_COMPONENTS_PER_CHUNK;

        assert(chunks[chunkIdx].GetEntityId(innerIdx) == id);
        
        return chunks[chunkIdx].GetComponent(innerIdx);
    }
}

void Scene::ComponentPool::FreeComponent(const EntityID id)
{
    const EntityIndex entityIdx = GetEntityIndex(id);

    assert(entityIdx < MAX_ENTITIES);

    if (sparseMap[entityIdx] == 0)
    {
        return;
    }
    
    const uint32_t chunkIdx = (sparseMap[entityIdx] - 1) / NUM_COMPONENTS_PER_CHUNK;
    const uint32_t innerIdx = (sparseMap[entityIdx] - 1) % NUM_COMPONENTS_PER_CHUNK;

    assert(chunks[chunkIdx].IsValid());
    assert(chunks[chunkIdx].GetEntityId(innerIdx) == id);
    
    chunks[chunkIdx].FreeComponent(innerIdx);

    // If the chunk is now empty we can free it
    if(chunks[chunkIdx].IsEmpty())
    {
        for(uint32_t i = NUM_CHUNKS_PER_POOL - 1; i >= chunkIdx; --i)
        {
            if(i == chunkIdx)
            {
                chunks[chunkIdx].~ComponentPoolChunk();
                break;
            }
            if(chunks[i].IsValid())
            {
                chunks[chunkIdx] = std::move(chunks[i]);
                // Update the sparse map to point to the correct chunk
                for(uint32_t j = 0; j < NUM_COMPONENTS_PER_CHUNK; ++j)
                {
                    if(!chunks[chunkIdx].freeComponents.test(j))
                    {
                        sparseMap[chunks[chunkIdx].GetEntityId(j)] = chunkIdx * NUM_COMPONENTS_PER_CHUNK + j;
                    }
                }
                break;
            }

        }
    }
    sparseMap[entityIdx] = 0;
}


#ifndef NDEBUG
void Scene::ComponentPool::DebugPrintState() const
{
    std::cout << "\tSparse Map:\n";
    for (uint32_t i = 0; i < MAX_ENTITIES; ++i)
    {
        if (sparseMap[i] != 0)
        {
            std::cout << "\t\tEntity " << i << ": " << sparseMap[i] - 1 << "\n";
        }
    }
    for (uint32_t chunkIndex = 0; chunkIndex < NUM_CHUNKS_PER_POOL; chunkIndex++)
    {
        std::cout << "Chunk " << chunkIndex << ":\n";
        if (chunks[chunkIndex].IsValid())
        {
            std::cout << "\t\t" << std::format("{:b}", chunks[chunkIndex].freeComponents.to_ullong()) << "\n";
        }
        else
        {
            std::cout << "\t\tUnused\n";
        }
    }
}

void Scene::DebugPrintState() const
{
    std::cout << "Entities: \n";
    for (auto [id, mask] : mEntities)
    {
        std::cout << "\t" << GetEntityIndex(id) << ", " << GetEntityVersion(id) << "\n";
        std::cout << "\t\t";
        for (uint32_t j = 0; j < mask.size(); ++j)
        {
            std::cout << mask.test(j);
        }
        std::cout << "\n";
    }

    std::cout << "Free Entities: \n";
    for (EntityIndex entityIdx : mFreeEntities)
    {
        std::cout << "\t" << entityIdx << "\n";
    }

    for (uint32_t i = 0; i < mComponentPools.size(); ++i)
    {
        if (mComponentPools[i])
        {
            std::cout << "Component Pool " << i << ":\n";
            mComponentPools[i]->DebugPrintState();
        }
    }
    std::cout.flush();
}
#endif