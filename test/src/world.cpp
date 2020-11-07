#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <execution>
#include <gtest/gtest.h>
#include <ecfw/core/world.hpp>
#include <ecfw/detail/entity.hpp>
#include <boost/range/adaptor/reversed.hpp>

const size_t NUM_ENTITIES = 100;

namespace dtl = ecfw::detail;

TEST(world, component_management) {
    struct C0 {};
    struct C1 {};
    struct C2 {};
    struct C3 {};

    ecfw::world world{};

    ASSERT_EQ(world.num_contained_types(), 0);
    ASSERT_FALSE((world.contains<C0, C1, C2, C3>()));

    world.reserve<C0>(NUM_ENTITIES);

    ASSERT_EQ(world.num_contained_types(), 1);
    ASSERT_TRUE((world.contains<C0>()));
    ASSERT_FALSE((world.contains<C1>()));
    ASSERT_FALSE((world.contains<C2>()));
    ASSERT_FALSE((world.contains<C3>()));

    auto entity = world.create();

    world.assign<C1>(entity);

    // An entity cannot posses unmanaged components
    ASSERT_FALSE(world.has<C2>(entity));

    ASSERT_EQ(world.num_contained_types(), 2);
    ASSERT_TRUE((world.contains<C0, C1>()));
    ASSERT_FALSE((world.contains<C2>()));
    ASSERT_FALSE((world.contains<C3>()));

    (void)world.create<C2>();

    ASSERT_EQ(world.num_contained_types(), 3);
    ASSERT_TRUE((world.contains<C0, C1, C2>()));
    ASSERT_FALSE((world.contains<C3>()));

    (void)world.view<C0, C1, C2, C3>();
    ASSERT_EQ(world.num_contained_types(), 4);
    ASSERT_TRUE((world.contains<C0, C1, C2, C3>()));
}

TEST(world, create_multiple_entities_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    world.create<C0, C1>(NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
}

TEST(
    world, create_multiple_entities_with_starting_components_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto c0_view = world.view<C0>();
    auto c1_view = world.view<C1>();
    auto c0c1_view = world.view<C0, C1>();

    world.create<C0>(NUM_ENTITIES);

    ASSERT_EQ(c0_view.size(), NUM_ENTITIES);
    ASSERT_EQ(c1_view.size(), 0);
    ASSERT_EQ(c0c1_view.size(), 0);

    world.create<C1>(NUM_ENTITIES);
    ASSERT_EQ(c0_view.size(), NUM_ENTITIES);
    ASSERT_EQ(c1_view.size(), NUM_ENTITIES);
    ASSERT_EQ(c0c1_view.size(), 0);
}

TEST(world, create_single_entity_no_starting_components) {
    ecfw::world world{};
    auto entity = world.create();
    
    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));
}

TEST(world, create_and_store_multiple_entities_no_starting_components) {
    ecfw::world world;
    std::vector<uint64_t> entities{};
    world.create(std::back_inserter(entities), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_batch_no_starting_components) {
    ecfw::world world;
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create(entities.begin(), entities.end());
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_single_entity_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);
}

TEST(world, create_and_store_multiple_entities_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_batch_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_single_entity_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_TRUE(world.valid(entity));

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);

    // ensure any new compatible entity
    // is picked up by the view
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);
    auto entity1 = world.create<C0, C1>();
    index = dtl::index(entity1);
    version = dtl::version(entity1);
    ASSERT_EQ(index, 1);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(v0.size(), 2);
    ASSERT_EQ(v1.size(), 2);
    ASSERT_EQ(v2.size(), 2);
    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_and_store_multiple_entities_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_batch_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, recycle_single_entity_no_starting_components) {
    ecfw::world world{};
    auto entity = world.create();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    world.destroy(entity);
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_reusable(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_FALSE(world.valid(entity));

    entity = world.create();
    index = dtl::index(entity);
    version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 1);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));
}

TEST(world, recycle_and_store_multiple_entities_no_starting_components) {
    ecfw::world world;
    std::vector<uint64_t> entities{};
    world.create(std::back_inserter(entities), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    world.create(std::begin(entities), NUM_ENTITIES);
    for (auto entity : entities) {
        auto version = dtl::version(entity);
        ASSERT_EQ(version, 1);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, recycle_batch_no_starting_components) {
    ecfw::world world;
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create(entities.begin(), entities.end());
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    world.create(entities.begin(), entities.end());
    for (auto entity : entities) {
        auto version = dtl::version(entity);
        ASSERT_EQ(version, 1);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
}

TEST(world, recycle_single_entity_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);

    world.destroy(entity);
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 1);

    // Ensure the entity is no longer valid
    ASSERT_FALSE(world.valid(entity));

    // Ensure that the components were destroyed successfully
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Create a new entity
    entity = world.create();

    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure that it's valid & has version # 1
    ASSERT_TRUE(world.valid(entity));
    ASSERT_EQ(dtl::version(entity), 1);

    // Ensure that no components were reconstructed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Recycle again this time with components
    world.destroy(entity);

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 1);

    entity = world.create<C0, C1>();
    ASSERT_TRUE(world.valid(entity));
    ASSERT_EQ(dtl::version(entity), 2);

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);
}

TEST(world, recycle_and_store_multiple_entities_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Check that the destroyed entities are now invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Ensure that the components were destroyed successfully
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Recycle the entities, but do not provide starting components
    world.create(std::begin(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure that each entity now has version # 1
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){ 
        return dtl::version(e) == 1; 
    }));

    // Ensure that no components were reconstructed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Check that the destroyed entities are now invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Recycle the entities and provide starting components
    world.create<C0, C1>(std::begin(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure that the recycled entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure that the recycled entities have version #2
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 2;
    }));

    // Ensure that only std::size(entities) components were
    // created of each type
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
}

TEST(world, recycle_batch_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Check that the destroyed entities are now invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Ensure that the components were destroyed successfully
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Recycle the entities, but do not provide starting components
    world.create(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure that each entity now has version # 1
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){ 
        return dtl::version(e) == 1; 
    }));

    // Ensure that no components were reconstructed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Check that the destroyed entities are now invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Recycle the entities and provide starting components
    world.create<C0, C1>(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure that the recycled entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure that the recycled entities have version #2
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 2;
    }));

    // Ensure that only std::size(entities) components were
    // created of each type
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
}

TEST(world, recycle_single_entity_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_TRUE(world.valid(entity));

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);

    // ensure the new  entity
    // is picked up by the view
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);

    // Ensure the entity's index & version are both 0
    auto entity1 = world.create<C0, C1>();

    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);

    index = dtl::index(entity1);
    version = dtl::version(entity1);
    ASSERT_EQ(index, 1);
    ASSERT_EQ(version, 0);

    ASSERT_EQ((world.count<C0>()), 2);
    ASSERT_EQ((world.count<C1>()), 2);
    ASSERT_EQ((world.count<C0, C1>()), 2);
    ASSERT_EQ(v0.size(), 2);
    ASSERT_EQ(v1.size(), 2);
    ASSERT_EQ(v2.size(), 2);

    // Destroy the entity & recycle it without starting components
    world.destroy(entity);

    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 1);

    // Ensure the entity is now invalid
    ASSERT_FALSE(world.valid(entity));

    // Ensure that it's components have been destroyed
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);

    entity = world.create();

    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure the entity is valid
    ASSERT_TRUE(world.valid(entity));

    // Ensure the entity has been reused. i.e., version is 1
    ASSERT_EQ(dtl::version(entity), 1);

    // Ensure that there is only one entity with the starting components
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);


    // Destroy the entity again & recycle with starting components
    world.destroy(entity);

    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 1);

    entity = world.create<C0, C1>();

    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure the entity is valid
    ASSERT_TRUE(world.valid(entity));

    // Ensure the entity has been reused. i.e., version is 2
    ASSERT_EQ(dtl::version(entity), 2);

    // Ensure that there is now two entities with the starting components
    ASSERT_EQ((world.count<C0>()), 2);
    ASSERT_EQ((world.count<C1>()), 2);
    ASSERT_EQ((world.count<C0, C1>()), 2);
    ASSERT_EQ(v0.size(), 2);
    ASSERT_EQ(v1.size(), 2);
    ASSERT_EQ(v2.size(), 2);
}

TEST(world, recycle_and_store_multiple_entities_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);

    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);


    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 2 * NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Ensure that all components have been destroyed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Ensure that all views are now empty
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);
    
    // Clear because std::size(entities) == 2 * NUM_ENTITIES
    entities.clear();

    // Recycle the entities without starting components
    world.create(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Ensure that the entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure they all have version # 1
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 1;
    }));

    // Ensure that components have not be reconstructed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Ensure that views are still empty
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 2 * NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Recycle the entities with starting components
    world.create<C0, C1>(std::begin(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Ensure that the entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure they all have version # 2
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 2;
    }));

    // Ensure that the components have been created
    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);

    // Ensure that the view picks up the entities
    ASSERT_EQ((world.count<C0, C1>()),NUM_ENTITIES);
}

TEST(world, recycle_batch_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Ensure that all components have been destroyed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // // Ensure that all views are now empty
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);

    // Recycle the entities without starting components
    world.create(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure that the entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure they all have version # 1
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 1;
    }));

    // Ensure that components have not be reconstructed
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);

    // Ensure that views are still empty
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Recycle the entities with starting components
    world.create<C0, C1>(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Ensure that the entities are valid
    ASSERT_TRUE(world.valid(entities.begin(), entities.end()));

    // Ensure they all have version # 2
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [](auto e){
        return dtl::version(e) == 2;
    }));

    // Ensure that the components have been created
    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);

    // Ensure that the view picks up the entities
    ASSERT_EQ((world.count<C0, C1>()),NUM_ENTITIES);
}

TEST(world, recycle_multiple_entities_with_starting_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities{};
    world.create(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    world.create<C0, C1>(NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
}

TEST(world, recycle_multiple_entities_with_starting_components_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto c0_view = world.view<C0>();
    auto c1_view = world.view<C1>();

    std::vector<uint64_t> entities{};

    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ(c0_view.size(), NUM_ENTITIES);
    ASSERT_EQ(c1_view.size(), NUM_ENTITIES);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);
    ASSERT_EQ(c0_view.size(), 0);
    ASSERT_EQ(c1_view.size(), 0);

    world.create<C0, C1>(NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ(c0_view.size(), NUM_ENTITIES);
    ASSERT_EQ(c1_view.size(), NUM_ENTITIES);
}

TEST(world, create_single_clone) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    auto clone = world.clone<C0, C1>(entity);

    index = dtl::index(clone);
    version = dtl::version(clone);
    ASSERT_EQ(index, 1);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(clone));
}

TEST(world, create_and_store_multiple_clones) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.clone<C0, C1>(entity, entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES + 1);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES + 1);
    ASSERT_EQ(world.num_reusable(), 0);

    world.clone<C0, C1>(entity, std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES + 1);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES + 1);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, create_multiple_clones_with_no_starting_views) {}

TEST(world, create_multiple_clones_with_starting_views) {}

TEST(world, destroy_single_entity_no_components) {
    ecfw::world world{};
    auto entity = world.create();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    world.destroy(entity);
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 1);
    ASSERT_FALSE(world.valid(entity));
}

TEST(world, destroy_multiple_entities_no_components) {
    ecfw::world world;
    std::vector<uint64_t> entities{};
    world.create(std::back_inserter(entities), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));
}

TEST(world, destroy_batch_no_components) {
    ecfw::world world;
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create(entities.begin(), entities.end());
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.destroy(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));
}

TEST(world, destroy_single_entities_with_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_TRUE(world.valid(entity));

    world.destroy(entity);
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 1);
    ASSERT_FALSE(world.valid(entity));
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
}

TEST(world, destroy_multiple_entities_with_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    
    world.destroy(entities.begin(), entities.end());
    
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
}

TEST(world, destroy_batch_with_components) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};
    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }
    
    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
    
    world.destroy(entities.begin(), entities.end());
    
    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
}

TEST(world, destroy_single_entity_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    auto entity = world.create<C0, C1>();

    // Ensure the entity's index & version are both 0
    auto index = dtl::index(entity);
    auto version = dtl::version(entity);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(version, 0);
    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 1);
    ASSERT_EQ(world.num_reusable(), 0);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_TRUE(world.valid(entity));

    // Ensure that the components were created successfully
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);

    // ensure the new  entity
    // is picked up by the view
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);

    auto entity1 = world.create<C0, C1>();
    index = dtl::index(entity1);
    version = dtl::version(entity1);
    ASSERT_EQ(index, 1);
    ASSERT_EQ(version, 0);

    ASSERT_EQ(world.num_alive(), 2);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ((world.count<C0>()), 2);
    ASSERT_EQ((world.count<C1>()), 2);
    ASSERT_EQ((world.count<C0, C1>()), 2);
    ASSERT_EQ(v0.size(), 2);
    ASSERT_EQ(v1.size(), 2);
    ASSERT_EQ(v2.size(), 2);

    // Destroy the entity & recycle it without starting components
    world.destroy(entity);

    ASSERT_EQ(world.num_alive(), 1);
    ASSERT_EQ(world.num_entities(), 2);
    ASSERT_EQ(world.num_reusable(), 1);

    // Ensure the entity is now invalid
    ASSERT_FALSE(world.valid(entity));

    // Ensure that it's components have been destroyed
    ASSERT_EQ((world.count<C0>()), 1);
    ASSERT_EQ((world.count<C1>()), 1);
    ASSERT_EQ((world.count<C0, C1>()), 1);
    ASSERT_EQ(v0.size(), 1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v2.size(), 1);
}

TEST(world, destroy_multiple_entities_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities{};
    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    world.create<C0, C1>(std::back_inserter(entities), NUM_ENTITIES);
    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 2 * NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);
}

TEST(world, destroy_batch_with_existing_views) {
    struct C0 {};
    struct C1 {};

    ecfw::world world{};

    auto v0 = world.view<C0, C1>();
    auto v1 = world.view<C0>();
    auto v2 = world.view<C1>();

    std::vector<uint64_t> entities(NUM_ENTITIES);
    world.create<C0, C1>(entities.begin(), entities.end());
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    uint32_t size = static_cast<uint32_t>(entities.size());
    for (uint32_t i = 0; i != size; ++i) {
        auto entity = entities[i];
        auto index = dtl::index(entity);
        auto version = dtl::version(entity);
        ASSERT_EQ(index, i);
        ASSERT_EQ(version, 0);
        ASSERT_TRUE(world.valid(entity));
    }

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);
    ASSERT_EQ(v2.size(), NUM_ENTITIES);

    // Destroy the entities
    world.destroy(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), 0);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), NUM_ENTITIES);

    // Ensure that they're all invalid
    ASSERT_FALSE(world.valid(entities.begin(), entities.end()));

    // Ensure that all components have been destroyed logically
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v2.size(), 0);
}

TEST(world, component_assignment_no_existing_views) {
    struct C0 {
        C0(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C1 {
        C1(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C2 {
        C2(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };

    std::vector<uint64_t> entities{};
    ecfw::world world{};

    auto v1 = world.view<C0, C1>();
    auto v3 = world.view<C1, C2>();
    auto v5 = world.view<C1>();

    world.create(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.assign<C0, C1, C2>(entities.begin(), entities.end());

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C0>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C1>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C2>(entity)));

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    auto v0 = world.view<C0, C1, C2>();
    auto v2 = world.view<C0, C2>();
    auto v4 = world.view<C0>();
    auto v6 = world.view<C2>();

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES);       
}

TEST(world, component_assignment_existing_views) {
    struct C0 {
        C0(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C1 {
        C1(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C2 {
        C2(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };

    std::vector<uint64_t> entities{};
    ecfw::world world{};

    auto v1 = world.view<C0, C1>();
    auto v3 = world.view<C1, C2>();
    auto v5 = world.view<C1>();

    world.create(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.assign<C0, C1, C2>(entities.begin(), entities.end());

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C0>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C1>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C2>(entity)));

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    auto v0 = world.view<C0, C1, C2>();
    auto v2 = world.view<C0, C2>();
    auto v4 = world.view<C0>();
    auto v6 = world.view<C2>();

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES);

    world.create(std::begin(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    for (auto entity : entities)
        ASSERT_TRUE(world.assign<C0>(entity, true));

    for (auto entity : entities)
        ASSERT_TRUE(world.assign<C1>(entity, true));

    for (auto entity : entities)
        ASSERT_TRUE(world.assign<C2>(entity, true));   

    ASSERT_EQ((world.count<C0, C1, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), 2 * NUM_ENTITIES);

    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), 2 * NUM_ENTITIES);
}

TEST(world, component_retrieval) {
    struct C0 {
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C1 {
        operator bool() const noexcept {return value;}
        bool value{false};
    };

    ecfw::world world{};
    auto entity = world.create<C0, C1>();

    auto&& [c0, c1] = world.get<C0, const C1>(entity);
    ASSERT_FALSE(c0);
    ASSERT_FALSE(c1);

    auto& cc0 = world.get<C0>(entity);
    auto& cc1 = world.get<C0>(entity);

    ASSERT_FALSE(cc0);
    ASSERT_FALSE(cc1);

    const ecfw::world& cref = world;

    auto [x, y] = cref.get<const C0, const C1>(entity);
    ASSERT_FALSE(x);
    ASSERT_FALSE(y);
}

TEST(world, component_removal_no_existing_views) {
    struct C0 {
        C0(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C1 {
        C1(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C2 {
        C2(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };

    std::vector<uint64_t> entities{};
    ecfw::world world{};

    auto v1 = world.view<C0, C1>();
    auto v3 = world.view<C1, C2>();
    auto v5 = world.view<C1>();

    world.create(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.assign<C0, C1, C2>(entities.begin(), entities.end());

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C0>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C1>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C2>(entity)));

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    auto v0 = world.view<C0, C1, C2>();
    auto v2 = world.view<C0, C2>();
    auto v4 = world.view<C0>();
    auto v6 = world.view<C2>();

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES); 

    world.remove<C0>(entities.begin(), entities.end());

    ASSERT_EQ((world.count<C0, C1, C2>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ((world.count<C0, C2>()), 0);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);   
    ASSERT_EQ(v2.size(), 0);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), 0);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES); 

    world.remove<C1>(entities.begin(), entities.end());

    ASSERT_EQ((world.count<C0, C1, C2>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ((world.count<C0, C2>()), 0);
    ASSERT_EQ((world.count<C1, C2>()), 0);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);   
    ASSERT_EQ(v2.size(), 0);   
    ASSERT_EQ(v3.size(), 0);   
    ASSERT_EQ(v4.size(), 0);   
    ASSERT_EQ(v5.size(), 0);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES); 

    world.remove<C2>(entities.begin(), entities.end());
    
    ASSERT_EQ((world.count<C0, C1, C2>()), 0);
    ASSERT_EQ((world.count<C0, C1>()), 0);
    ASSERT_EQ((world.count<C1, C2>()), 0);
    ASSERT_EQ((world.count<C0, C2>()), 0);
    ASSERT_EQ((world.count<C0>()), 0);
    ASSERT_EQ((world.count<C1>()), 0);
    ASSERT_EQ((world.count<C2>()), 0);

    ASSERT_EQ(v0.size(), 0);
    ASSERT_EQ(v1.size(), 0);   
    ASSERT_EQ(v2.size(), 0);   
    ASSERT_EQ(v3.size(), 0);   
    ASSERT_EQ(v4.size(), 0);   
    ASSERT_EQ(v5.size(), 0);   
    ASSERT_EQ(v6.size(), 0); 
}

TEST(world, component_removal_existing_views) {
    struct C0 {
        C0(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C1 {
        C1(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    struct C2 {
        C2(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };

    std::vector<uint64_t> entities{};
    ecfw::world world{};

    auto v1 = world.view<C0, C1>();
    auto v3 = world.view<C1, C2>();
    auto v5 = world.view<C1>();

    world.create(std::back_inserter(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.assign<C0, C1, C2>(entities.begin(), entities.end());

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C0>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C1>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C2>(entity)));

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    auto v0 = world.view<C0, C1, C2>();
    auto v2 = world.view<C0, C2>();
    auto v4 = world.view<C0>();
    auto v6 = world.view<C2>();

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES); 

    world.create(std::begin(entities), NUM_ENTITIES);

    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);

    world.assign<C0, C1, C2>(entities.begin(), entities.end());

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C0>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C1>(entity)));

    for (auto entity : entities)
        ASSERT_FALSE((world.get<C2>(entity)));

    ASSERT_EQ((world.count<C0, C1, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), 2 * NUM_ENTITIES);

    ASSERT_EQ(v0.size(), 2 * NUM_ENTITIES);
    ASSERT_EQ(v1.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), 2 * NUM_ENTITIES); 

    world.remove<C0>(entities.begin(), entities.end());

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), 2 * NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), 2 * NUM_ENTITIES);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), 2 * NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), 2 * NUM_ENTITIES); 

    world.remove<C1>(entities.begin(), entities.end());

    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), 2 * NUM_ENTITIES);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), 2 * NUM_ENTITIES); 

    world.remove<C2>(entities.begin(), entities.end());
    
    ASSERT_EQ((world.count<C0, C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1, C2>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C0>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C1>()), NUM_ENTITIES);
    ASSERT_EQ((world.count<C2>()), NUM_ENTITIES);

    ASSERT_EQ(v0.size(), NUM_ENTITIES);
    ASSERT_EQ(v1.size(), NUM_ENTITIES);   
    ASSERT_EQ(v2.size(), NUM_ENTITIES);   
    ASSERT_EQ(v3.size(), NUM_ENTITIES);   
    ASSERT_EQ(v4.size(), NUM_ENTITIES);   
    ASSERT_EQ(v5.size(), NUM_ENTITIES);   
    ASSERT_EQ(v6.size(), NUM_ENTITIES); 

    ASSERT_EQ(world.num_alive(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_entities(), 2 * NUM_ENTITIES);
    ASSERT_EQ(world.num_reusable(), 0);
}

TEST(world, capacity) {
    struct C0 { double data{0.0}; };

    std::vector<uint64_t> entities(NUM_ENTITIES);
    ecfw::world world{};
    world.create<C0>(entities.begin(), entities.end());
    ASSERT_EQ(world.size<C0>(), NUM_ENTITIES);
    ASSERT_NE(world.size<C0>(), world.capacity<C0>());
    world.shrink_to_fit<C0>();
    ASSERT_EQ(world.size<C0>(), world.capacity<C0>());
}

TEST(world, component_replacement) {
    struct C0 {
        C0(bool value = false) : value(value) {}
        operator bool() const noexcept {return value;}
        bool value{false};
    };
    ecfw::world world{};
    auto entity = world.create();
    ASSERT_FALSE((world.assign_or_replace<C0>(entity)));
    ASSERT_TRUE((world.assign_or_replace<C0>(entity, true)));
}

TEST(world, reserve_component_storage) {
    struct C0 {
        C0(bool value = false)
            : value(value)
        {}
        operator bool() const {
            return value;
        }
        bool value = false;
    };
    struct C1 {
        C1(bool value = false)
            : value(value)
        {}
        operator bool() const {
            return value;
        }
        bool value = false;
    };
    
    ecfw::world world{};
    std::vector<uint64_t> entities{};
    world.create(std::back_inserter(entities), NUM_ENTITIES);
    world.reserve<C0, C1>(NUM_ENTITIES);

    for (auto entity : entities) {
        ASSERT_TRUE((world.assign<C0>(entity, true)));
        ASSERT_TRUE((world.assign<C1>(entity, true)));
    }
}

TEST(world, view_creation) {
    struct C0 { bool state{false}; };
    struct C1 { bool state{false}; };
    struct C2 { bool state{false}; };
    ecfw::world world{};

    (void)world.create<C0, C1, C2>();

    auto view = world.view<C0, const C1, C2>();

    ASSERT_EQ(view.size(), 1);

    const ecfw::world& cref = world;
    auto readonly_view = cref.view<const C0, const C1, const C2>();

    ASSERT_EQ(view.size(), 1);
}

TEST(world, view_creation_consistency) {
    using std::equal;

    struct C0 { bool state{false}; };
    struct C1 { bool state{false}; };
    struct C2 { bool state{false}; };

    ecfw::world world{};
    std::vector<uint64_t> entities(NUM_ENTITIES);

    auto v0 = world.view<C0, C1, C2>();
    auto v1 = world.view<C0, C2, C1>();
    auto v2 = world.view<C1, C0, C2>();
    auto v3 = world.view<C1, C2, C0>();
    auto v4 = world.view<C2, C0, C1>();
    auto v5 = world.view<C2, C1, C0>();

    world.create(entities.begin(), entities.end());

    for (size_t i = 0; i < entities.size(); ++i)
        world.assign<C0>(entities[i]);
    
    for (size_t i = 0; i < entities.size() / 2; ++i)
        world.assign<C1>(entities[i]);

    for (size_t i = 0; i < entities.size() / 4; ++i)
        world.assign<C2>(entities[i]);

    ASSERT_EQ(v0.size(), NUM_ENTITIES / 4);
    ASSERT_EQ(v1.size(), NUM_ENTITIES / 4);
    ASSERT_EQ(v2.size(), NUM_ENTITIES / 4);
    ASSERT_EQ(v3.size(), NUM_ENTITIES / 4);
    ASSERT_EQ(v4.size(), NUM_ENTITIES / 4);
    ASSERT_EQ(v5.size(), NUM_ENTITIES / 4);

    ASSERT_TRUE(equal(v0.begin(), v0.end(), v1.begin(), v1.end()));
    ASSERT_TRUE(equal(v0.begin(), v0.end(), v2.begin(), v2.end()));
    ASSERT_TRUE(equal(v0.begin(), v0.end(), v3.begin(), v3.end()));
    ASSERT_TRUE(equal(v0.begin(), v0.end(), v4.begin(), v4.end()));
    ASSERT_TRUE(equal(v0.begin(), v0.end(), v5.begin(), v5.end()));

    ASSERT_TRUE(equal(v1.begin(), v1.end(), v2.begin(), v2.end()));
    ASSERT_TRUE(equal(v1.begin(), v1.end(), v3.begin(), v3.end()));
    ASSERT_TRUE(equal(v1.begin(), v1.end(), v4.begin(), v4.end()));
    ASSERT_TRUE(equal(v1.begin(), v1.end(), v5.begin(), v5.end()));

    ASSERT_TRUE(equal(v2.begin(), v2.end(), v3.begin(), v3.end()));
    ASSERT_TRUE(equal(v2.begin(), v2.end(), v4.begin(), v4.end()));
    ASSERT_TRUE(equal(v2.begin(), v2.end(), v5.begin(), v5.end()));

    ASSERT_TRUE(equal(v3.begin(), v3.end(), v4.begin(), v4.end()));
    ASSERT_TRUE(equal(v3.begin(), v3.end(), v5.begin(), v5.end()));

    ASSERT_TRUE(equal(v4.begin(), v4.end(), v5.begin(), v5.end()));
}

TEST(single_component_view, componen_retrieval) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    ecfw::world world{};
    auto entity = world.create<B0>();
    auto view = world.view<B0>();

    ASSERT_FALSE(view.get(entity));
}

TEST(single_component_view, sequential_forward_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0>();

    // Update the value of the component
    for (auto entity : view) {
        auto& b0 = view.get(entity);
        ASSERT_FALSE(b0);
        b0.value = true;
        ASSERT_TRUE(b0);
    }

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e);
    }));

}

TEST(single_component_view, sequential_reverse_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0>();

    // Update the value of the component in reverse
    for (auto entity : boost::adaptors::reverse(view)) {
        auto& b0 = view.get(entity);
        ASSERT_FALSE(b0);
        b0.value = true;
        ASSERT_TRUE(b0);
    }

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e);
    }));
}

TEST(single_component_view, parallel_forward_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0>();

    std::for_each(std::execution::par, view.begin(), view.end(), [&view](auto entity) {
        auto& b0 = view.get(entity);
        ASSERT_FALSE(b0);
        b0.value = true;
        ASSERT_TRUE(b0);
    });

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e);
    }));
}

TEST(single_component_view, parallel_reverse_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0>();

    std::for_each(std::execution::par, view.rbegin(), view.rend(), [&view](auto entity) {
        auto& b0 = view.get(entity);
        ASSERT_FALSE(b0);
        b0.value = true;
        ASSERT_TRUE(b0);
    });

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e) {
        return world.get<B0>(e);
    }));
}

TEST(multi_component_view, component_retrieval) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B1 {
        B1() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B2 {
        B2() {}
        operator bool() const { return value; }
        bool value{false};
    };
    ecfw::world world{};
    auto entity = world.create<B0, B1, B2>();
    auto view = world.view<B0, const B1, B2>();

    auto&& [b0, b1, b2] = view.get(entity);
    ASSERT_FALSE(b0);
    ASSERT_FALSE(b1);
    ASSERT_FALSE(b2);
    ASSERT_FALSE(view.get<B0>(entity));
    ASSERT_FALSE(view.get<const B1>(entity));
    auto&& [bb1, bb2] = view.get<const B1, B2>(entity);
    ASSERT_FALSE(bb1);
    ASSERT_FALSE(bb2);

    const auto& cref = world;
    auto  const_view = cref.view<const B0, const B1, const B2>();
    auto&& [cb0, cb1, cb2] = const_view.get(entity);
    ASSERT_FALSE(cb0);
    ASSERT_FALSE(cb1);
    ASSERT_FALSE(cb2);
    ASSERT_FALSE(const_view.get<const B0>(entity));
    ASSERT_FALSE(const_view.get<const B1>(entity));
}

TEST(multi_component_view, sequential_forward_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B1 {
        B1() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B2 {
        B2() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0, B1, B2>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0, B1, B2>();

    // Update the value of the component
    for (auto entity : view) {
        auto&& [b0, b1, b2] = view.get(entity);
        ASSERT_FALSE(b0);
        ASSERT_FALSE(b1);
        ASSERT_FALSE(b2);
        b0.value = true;
        b1.value = true;
        b2.value = true;
        ASSERT_TRUE(b0);
        ASSERT_TRUE(b1);
        ASSERT_TRUE(b2);
    }

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e) && world.get<B1>(e) && world.get<B2>(e);
    }));
}

TEST(multi_component_view, sequential_reverse_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B1 {
        B1() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B2 {
        B2() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0, B1, B2>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0, B1, B2>();

    // Update the value of the component
    for (auto entity : boost::adaptors::reverse(view)) {
        auto&& [b0, b1, b2] = view.get(entity);
        ASSERT_FALSE(b0);
        ASSERT_FALSE(b1);
        ASSERT_FALSE(b2);
        b0.value = true;
        b1.value = true;
        b2.value = true;
        ASSERT_TRUE(b0);
        ASSERT_TRUE(b1);
        ASSERT_TRUE(b2);
    }

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e) && world.get<B1>(e) && world.get<B2>(e);
    }));
}

TEST(multi_component_view, parallel_forward_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B1 {
        B1() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B2 {
        B2() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0, B1, B2>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0, B1, B2>();

    // Update the value of the component
    std::for_each(std::execution::par, view.begin(), view.end(), [&view](auto entity){
        auto&& [b0, b1, b2] = view.get(entity);
        ASSERT_FALSE(b0);
        ASSERT_FALSE(b1);
        ASSERT_FALSE(b2);
        b0.value = true;
        b1.value = true;
        b2.value = true;
        ASSERT_TRUE(b0);
        ASSERT_TRUE(b1);
        ASSERT_TRUE(b2);
    });

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e) && world.get<B1>(e) && world.get<B2>(e);
    }));
}

TEST(multi_component_view, parallel_reverse_iteration) {
    struct B0 {
        B0() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B1 {
        B1() {}
        operator bool() const { return value; }
        bool value{false};
    };
    struct B2 {
        B2() {}
        operator bool() const { return value; }
        bool value{false};
    };
    std::vector<uint64_t> entities{};
    ecfw::world world{};
    world.create<B0, B1, B2>(std::back_inserter(entities), NUM_ENTITIES);
    auto view = world.view<B0, B1, B2>();

    // Update the value of the component
    std::for_each(std::execution::par, view.rbegin(), view.rend(), [&view](auto entity){
        auto&& [b0, b1, b2] = view.get(entity);
        ASSERT_FALSE(b0);
        ASSERT_FALSE(b1);
        ASSERT_FALSE(b2);
        b0.value = true;
        b1.value = true;
        b2.value = true;
        ASSERT_TRUE(b0);
        ASSERT_TRUE(b1);
        ASSERT_TRUE(b2);
    });

    // Ensure each entity had their component updated
    ASSERT_TRUE(std::all_of(entities.begin(), entities.end(), [&world](auto e){
        return world.get<B0>(e) && world.get<B1>(e) && world.get<B2>(e);
    }));
}