#pragma once

#include <iostream>
#include <ctime>

#include "Test.h"
#include "..\Engine\Components\Entity.h"
#include "..\Engine\Components\Transform.h"

using namespace lightning;

class EngineTest : public Test {
	private:
		util::vector<game_entity::Entity> _entities;

		u32 _added{ 0 };
		u32 _removed{ 0 };
		u32 _num_entities{ 0 };

		void create_random() {
			u32 count = rand() % 20;
			if (_entities.empty()) count = 1000;
			transform::InitInfo transform_info{};
			game_entity::EntityInfo entity_info{ &transform_info };
			while (count > 0) {
				++_added;
				game_entity::Entity entity{ game_entity::create(entity_info) };
				assert(entity.is_valid() && id::is_valid(entity.get_id()));
				_entities.push_back(entity);
				assert(game_entity::is_alive(entity.get_id()));
				--count;
			}
		}
		void remove_random() {
			u32 count = rand() % 20;
			if (_entities.size() < 1000) return;
			while (count > 0) {
				const u32 index((u32)rand() % (u32)_entities.size());
				const game_entity::Entity entity{ _entities[index] };
				assert(entity.is_valid() && id::is_valid(entity.get_id()));
				if (entity.is_valid()) {
					game_entity::remove(entity.get_id());
					_entities.erase(_entities.begin() + index);
					assert(!game_entity::is_alive(entity.get_id()));
					++_removed;
				}
				--count;
			}
		}

		void print_result() {
			std::cout << "Entities created: " << _added << '\n';
			std::cout << "Entities removed: " << _removed << '\n';
		}

	public:
		bool initialize() override {
			srand((u32)time(nullptr));
			return true;
		}
		void run() override {
			do {
				for (u32 i { 0 }; i < 10000; ++i) {
					create_random();
					remove_random();
					_num_entities = (u32)_entities.size();
				}
				print_result();
			} while (getchar() != 'q');
		}
		void shutdown() override {}
};