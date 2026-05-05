#include "Database.h"
#include <chrono>
#include <iostream>

namespace MMO {

	Database::Database() = default;
	Database::~Database() = default;

	bool Database::Connect(const std::string& connectionString)
	{
		try
		{
			m_Connection = std::make_unique<pqxx::connection>(connectionString);
			if (m_Connection->is_open())
			{
				std::cout << "Connected to database: " << m_Connection->dbname() << std::endl;
				return true;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Database connection failed: " << e.what() << std::endl;
		}
		return false;
	}

	void Database::Disconnect()
	{
		if (m_Connection)
		{
			m_Connection.reset();
		}
	}

	// ============================================================
	// ACCOUNT OPERATIONS
	// ============================================================

	std::optional<AccountData> Database::GetAccountByUsername(const std::string& username)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT id, username, password_hash, salt, is_banned, COALESCE(ban_reason, '') "
				"FROM accounts WHERE username = $1",
				username);
			txn.commit();

			if (result.empty())
			{
				return std::nullopt;
			}

			AccountData account;
			account.id = result[0][0].as<AccountId>();
			account.username = result[0][1].as<std::string>();
			account.passwordHash = result[0][2].as<std::string>();
			account.salt = result[0][3].as<std::string>();
			account.isBanned = result[0][4].as<bool>();
			account.banReason = result[0][5].as<std::string>();
			return account;
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetAccountByUsername failed: " << e.what() << std::endl;
			return std::nullopt;
		}
	}

	bool Database::CreateAccount(const std::string& username, const std::string& email,
								 const std::string& passwordHash, const std::string& salt)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params(
				"INSERT INTO accounts (username, email, password_hash, salt) VALUES ($1, $2, $3, $4)",
				username, email, passwordHash, salt);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "CreateAccount failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::UpdateLastLogin(AccountId accountId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params(
				"UPDATE accounts SET last_login = CURRENT_TIMESTAMP WHERE id = $1",
				accountId);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "UpdateLastLogin failed: " << e.what() << std::endl;
			return false;
		}
	}

	// ============================================================
	// CHARACTER OPERATIONS
	// ============================================================

	std::vector<CharacterData> Database::GetCharactersByAccountId(AccountId accountId)
	{
		std::vector<CharacterData> characters;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT id, account_id, name, race, class, level, experience, money, "
				"zone_id::INTEGER, "
				"position_x, position_y, position_z, orientation, "
				"max_health, max_mana, current_health, current_mana, "
				"EXTRACT(EPOCH FROM COALESCE(last_played, created_at))::BIGINT "
				"FROM characters WHERE account_id = $1 AND is_deleted = FALSE "
				"ORDER BY last_played DESC NULLS LAST",
				accountId);
			txn.commit();

			for (const auto& row : result)
			{
				CharacterData c;
				c.id = row[0].as<CharacterId>();
				c.accountId = row[1].as<AccountId>();
				c.name = row[2].as<std::string>();
				c.characterRace = static_cast<CharacterRace>(row[3].as<int>());
				c.characterClass = static_cast<CharacterClass>(row[4].as<int>());
				c.level = row[5].as<uint32_t>();
				c.experience = row[6].as<uint64_t>();
				c.money = row[7].as<uint32_t>();
				c.mapId = row[8].as<uint32_t>();
				c.positionX = row[9].as<float>();
				c.positionY = row[10].as<float>();
				c.positionZ = row[11].as<float>();
				c.orientation = row[12].as<float>();
				c.maxHealth = row[13].as<int32_t>();
				c.maxMana = row[14].as<int32_t>();
				c.currentHealth = row[15].as<int32_t>();
				c.currentMana = row[16].as<int32_t>();
				c.lastPlayed = row[17].as<uint64_t>();
				characters.push_back(c);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetCharactersByAccountId failed: " << e.what() << std::endl;
		}
		return characters;
	}

	std::optional<CharacterData> Database::GetCharacterById(CharacterId characterId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT id, account_id, name, race, class, level, experience, money, "
				"zone_id::INTEGER, "
				"position_x, position_y, position_z, orientation, "
				"max_health, max_mana, current_health, current_mana, "
				"EXTRACT(EPOCH FROM COALESCE(last_played, created_at))::BIGINT "
				"FROM characters WHERE id = $1 AND is_deleted = FALSE",
				characterId);
			txn.commit();

			if (result.empty())
			{
				return std::nullopt;
			}

			const auto& row = result[0];
			CharacterData c;
			c.id = row[0].as<CharacterId>();
			c.accountId = row[1].as<AccountId>();
			c.name = row[2].as<std::string>();
			c.characterRace = static_cast<CharacterRace>(row[3].as<int>());
			c.characterClass = static_cast<CharacterClass>(row[4].as<int>());
			c.level = row[5].as<uint32_t>();
			c.experience = row[6].as<uint64_t>();
			c.money = row[7].as<uint32_t>();
			c.mapId = row[8].as<uint32_t>();
			c.positionX = row[9].as<float>();
			c.positionY = row[10].as<float>();
			c.positionZ = row[11].as<float>();
			c.orientation = row[12].as<float>();
			c.maxHealth = row[13].as<int32_t>();
			c.maxMana = row[14].as<int32_t>();
			c.currentHealth = row[15].as<int32_t>();
			c.currentMana = row[16].as<int32_t>();
			c.lastPlayed = row[17].as<uint64_t>();
			return c;
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetCharacterById failed: " << e.what() << std::endl;
			return std::nullopt;
		}
	}

	bool Database::CreateCharacter(AccountId accountId, const std::string& name,
								   CharacterRace characterRace, CharacterClass characterClass,
								   uint32_t mapId, float posX, float posY, float posZ, float orientation,
								   int32_t maxHealth, int32_t maxMana,
								   CharacterId& outId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"INSERT INTO characters (account_id, name, race, class, zone_id, "
				"position_x, position_y, position_z, orientation, "
				"max_health, max_mana, current_health, current_mana) "
				"VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $10, $11) RETURNING id",
				accountId, name,
				static_cast<int>(characterRace), static_cast<int>(characterClass),
				std::to_string(mapId), posX, posY, posZ, orientation, maxHealth, maxMana);
			txn.commit();

			if (!result.empty())
			{
				outId = result[0][0].as<CharacterId>();
				return true;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "CreateCharacter failed: " << e.what() << std::endl;
		}
		return false;
	}

	bool Database::DeleteCharacter(CharacterId characterId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params(
				"UPDATE characters SET is_deleted = TRUE WHERE id = $1",
				characterId);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "DeleteCharacter failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::SaveCharacter(const CharacterData& character)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params(
				"UPDATE characters SET "
				"level = $2, experience = $3, money = $4, zone_id = $5, "
				"position_x = $6, position_y = $7, position_z = $8, orientation = $9, "
				"max_health = $10, max_mana = $11, current_health = $12, current_mana = $13, "
				"last_played = CURRENT_TIMESTAMP "
				"WHERE id = $1",
				character.id, character.level, character.experience, character.money,
				std::to_string(character.mapId),
				character.positionX, character.positionY, character.positionZ, character.orientation,
				character.maxHealth, character.maxMana,
				character.currentHealth, character.currentMana);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "SaveCharacter failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::IsNameTaken(const std::string& name)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1) AND is_deleted = FALSE",
				name);
			txn.commit();
			return !result.empty();
		}
		catch (const std::exception& e)
		{
			std::cerr << "IsNameTaken failed: " << e.what() << std::endl;
			return true; // Fail safe
		}
	}

	// ============================================================
	// COOLDOWN OPERATIONS
	// ============================================================

	std::vector<CooldownData> Database::GetCooldowns(CharacterId characterId)
	{
		std::vector<CooldownData> cooldowns;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT ability_id, remaining FROM character_cooldowns WHERE character_id = $1",
				characterId);
			txn.commit();

			for (const auto& row : result)
			{
				CooldownData cd;
				cd.abilityId = static_cast<AbilityId>(row[0].as<int>());
				cd.remaining = row[1].as<float>();
				cooldowns.push_back(cd);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetCooldowns failed: " << e.what() << std::endl;
		}
		return cooldowns;
	}

	bool Database::SaveCooldowns(CharacterId characterId, const std::vector<CooldownData>& cooldowns)
	{
		try
		{
			pqxx::work txn(*m_Connection);

			// Clear existing cooldowns
			txn.exec_params("DELETE FROM character_cooldowns WHERE character_id = $1", characterId);

			// Insert new cooldowns
			for (const auto& cd : cooldowns)
			{
				if (cd.remaining > 0.0f)
				{
					txn.exec_params(
						"INSERT INTO character_cooldowns (character_id, ability_id, remaining) "
						"VALUES ($1, $2, $3)",
						characterId, static_cast<int>(cd.abilityId), cd.remaining);
				}
			}

			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "SaveCooldowns failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::ClearCooldowns(CharacterId characterId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params("DELETE FROM character_cooldowns WHERE character_id = $1", characterId);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "ClearCooldowns failed: " << e.what() << std::endl;
			return false;
		}
	}

	// ============================================================
	// SESSION OPERATIONS
	// ============================================================

	bool Database::CreateSession(const std::string& token, AccountId accountId,
								 const std::string& ipAddress, int expiresInHours)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params(
				"INSERT INTO sessions (token, account_id, ip_address, expires_at) "
				"VALUES ($1, $2, $3, CURRENT_TIMESTAMP + INTERVAL '1 hour' * $4)",
				token, accountId, ipAddress, expiresInHours);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "CreateSession failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::ValidateSession(const std::string& token, AccountId& outAccountId)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT account_id FROM sessions WHERE token = $1 AND expires_at > CURRENT_TIMESTAMP",
				token);
			txn.commit();

			if (!result.empty())
			{
				outAccountId = result[0][0].as<AccountId>();
				return true;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "ValidateSession failed: " << e.what() << std::endl;
		}
		return false;
	}

	bool Database::DeleteSession(const std::string& token)
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec_params("DELETE FROM sessions WHERE token = $1", token);
			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "DeleteSession failed: " << e.what() << std::endl;
			return false;
		}
	}

	void Database::CleanupExpiredSessions()
	{
		try
		{
			pqxx::work txn(*m_Connection);
			txn.exec("DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP");
			txn.commit();
		}
		catch (const std::exception& e)
		{
			std::cerr << "CleanupExpiredSessions failed: " << e.what() << std::endl;
		}
	}

	// ============================================================
	// INVENTORY OPERATIONS
	// ============================================================

	std::vector<Database::InventoryItemData> Database::GetInventory(CharacterId characterId)
	{
		std::vector<InventoryItemData> items;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT instance_id, template_id, slot, stack_count "
				"FROM character_inventory WHERE character_id = $1 ORDER BY slot",
				characterId);
			txn.commit();

			for (const auto& row : result)
			{
				InventoryItemData item;
				item.instanceId = row[0].as<ItemInstanceId>();
				item.templateId = static_cast<ItemId>(row[1].as<uint32_t>());
				item.slot = static_cast<uint8_t>(row[2].as<int>());
				item.stackCount = row[3].as<uint32_t>();
				items.push_back(item);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetInventory failed: " << e.what() << std::endl;
		}
		return items;
	}

	std::vector<Database::EquipmentItemData> Database::GetEquipment(CharacterId characterId)
	{
		std::vector<EquipmentItemData> items;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT instance_id, template_id, slot "
				"FROM character_equipment WHERE character_id = $1 ORDER BY slot",
				characterId);
			txn.commit();

			for (const auto& row : result)
			{
				EquipmentItemData item;
				item.instanceId = row[0].as<ItemInstanceId>();
				item.templateId = static_cast<ItemId>(row[1].as<uint32_t>());
				item.slot = static_cast<uint8_t>(row[2].as<int>());
				items.push_back(item);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "GetEquipment failed: " << e.what() << std::endl;
		}
		return items;
	}

	bool Database::SaveInventory(CharacterId characterId, const std::vector<InventoryItemData>& items)
	{
		try
		{
			pqxx::work txn(*m_Connection);

			// Clear existing inventory
			txn.exec_params("DELETE FROM character_inventory WHERE character_id = $1", characterId);

			// Insert new items
			for (const auto& item : items)
			{
				txn.exec_params(
					"INSERT INTO character_inventory (instance_id, character_id, slot, template_id, stack_count) "
					"VALUES ($1, $2, $3, $4, $5)",
					item.instanceId, characterId, static_cast<int>(item.slot),
					static_cast<int>(item.templateId), item.stackCount);
			}

			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "SaveInventory failed: " << e.what() << std::endl;
			return false;
		}
	}

	bool Database::SaveEquipment(CharacterId characterId, const std::vector<EquipmentItemData>& items)
	{
		try
		{
			pqxx::work txn(*m_Connection);

			// Clear existing equipment
			txn.exec_params("DELETE FROM character_equipment WHERE character_id = $1", characterId);

			// Insert new items
			for (const auto& item : items)
			{
				txn.exec_params(
					"INSERT INTO character_equipment (instance_id, character_id, slot, template_id) "
					"VALUES ($1, $2, $3, $4)",
					item.instanceId, characterId, static_cast<int>(item.slot),
					static_cast<int>(item.templateId));
			}

			txn.commit();
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "SaveEquipment failed: " << e.what() << std::endl;
			return false;
		}
	}
	// ============================================================
	// MAP LOADING OPERATIONS (Server reads from DB)
	// ============================================================

	std::vector<MapTemplateData> Database::LoadAllMapTemplates()
	{
		std::vector<MapTemplateData> templates;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec("SELECT id, name, width, height, spawn_x, spawn_y, spawn_z FROM map_template ORDER BY id");
			txn.commit();

			for (const auto& row : result)
			{
				MapTemplateData t;
				t.id = row[0].as<uint32_t>();
				t.name = row[1].as<std::string>();
				t.width = row[2].as<float>();
				t.height = row[3].as<float>();
				t.spawnX = row[4].as<float>();
				t.spawnY = row[5].as<float>();
				t.spawnZ = row[6].as<float>();
				templates.push_back(t);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadAllMapTemplates failed: " << e.what() << std::endl;
		}
		return templates;
	}

	std::vector<PortalData> Database::LoadPortals(uint32_t mapId)
	{
		std::vector<PortalData> portals;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT id, position_x, position_y, size_x, size_y, dest_map_id, dest_x, dest_y "
				"FROM portal WHERE map_id = $1 ORDER BY id",
				mapId);
			txn.commit();

			for (const auto& row : result)
			{
				PortalData p;
				p.id = row[0].as<uint32_t>();
				p.positionX = row[1].as<float>();
				p.positionY = row[2].as<float>();
				p.sizeX = row[3].as<float>();
				p.sizeY = row[4].as<float>();
				p.destMapId = row[5].as<uint32_t>();
				p.destX = row[6].as<float>();
				p.destY = row[7].as<float>();
				portals.push_back(p);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadPortals failed: " << e.what() << std::endl;
		}
		return portals;
	}

	std::vector<CreatureSpawnData> Database::LoadCreatureSpawns(uint32_t mapId)
	{
		std::vector<CreatureSpawnData> spawns;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec_params(
				"SELECT guid, entry, position_x, position_y, position_z, orientation, "
				"       respawn_secs, wander_radius, max_count "
				"FROM creature_spawn WHERE map_id = $1 ORDER BY guid",
				mapId);
			txn.commit();

			for (const auto& row : result)
			{
				CreatureSpawnData s;
				s.guid = row[0].as<std::string>();
				s.creatureTemplateId = row[1].as<uint32_t>();
				s.positionX = row[2].as<float>();
				s.positionY = row[3].as<float>();
				s.positionZ = row[4].as<float>();
				s.orientation = row[5].as<float>();
				s.respawnTime = row[6].as<float>();
				s.wanderRadius = row[7].as<float>();
				s.maxCount = row[8].as<uint32_t>();
				spawns.push_back(s);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadCreatureSpawns failed: " << e.what() << std::endl;
		}
		return spawns;
	}

	// ============================================================
	// RACE/CLASS TEMPLATE LOADING
	// ============================================================

	std::vector<RaceTemplate> Database::LoadRaceTemplates()
	{
		std::vector<RaceTemplate> races;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec(
				"SELECT id, name, faction, class_mask, bonus_strength, bonus_agility, bonus_stamina, bonus_intellect "
				"FROM race_template ORDER BY id");
			txn.commit();

			for (const auto& row : result)
			{
				RaceTemplate r;
				r.id = static_cast<CharacterRace>(row[0].as<int>());
				r.name = row[1].as<std::string>();
				r.faction = static_cast<uint8_t>(row[2].as<int>());
				r.classMask = row[3].as<uint32_t>();
				r.bonusStrength = row[4].as<int32_t>();
				r.bonusAgility = row[5].as<int32_t>();
				r.bonusStamina = row[6].as<int32_t>();
				r.bonusIntellect = row[7].as<int32_t>();
				races.push_back(r);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadRaceTemplates failed: " << e.what() << std::endl;
		}
		return races;
	}

	std::vector<ClassTemplate> Database::LoadClassTemplates()
	{
		std::vector<ClassTemplate> classes;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec(
				"SELECT id, name, base_health, base_mana, base_strength, base_agility, base_stamina, base_intellect "
				"FROM class_template ORDER BY id");
			txn.commit();

			for (const auto& row : result)
			{
				ClassTemplate c;
				c.id = static_cast<CharacterClass>(row[0].as<int>());
				c.name = row[1].as<std::string>();
				c.baseHealth = row[2].as<int32_t>();
				c.baseMana = row[3].as<int32_t>();
				c.baseStrength = row[4].as<int32_t>();
				c.baseAgility = row[5].as<int32_t>();
				c.baseStamina = row[6].as<int32_t>();
				c.baseIntellect = row[7].as<int32_t>();
				classes.push_back(c);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadClassTemplates failed: " << e.what() << std::endl;
		}
		return classes;
	}

	std::vector<Database::PlayerCreateInfoRow> Database::LoadPlayerCreateInfo()
	{
		std::vector<PlayerCreateInfoRow> infos;
		try
		{
			pqxx::work txn(*m_Connection);
			pqxx::result result = txn.exec(
				"SELECT race, class, map_id, position_x, position_y, position_z, orientation "
				"FROM player_create_info ORDER BY race, class");
			txn.commit();

			for (const auto& row : result)
			{
				PlayerCreateInfoRow r;
				r.race = static_cast<uint8_t>(row[0].as<int>());
				r.cls = static_cast<uint8_t>(row[1].as<int>());
				r.info.mapId = row[2].as<uint32_t>();
				r.info.positionX = row[3].as<float>();
				r.info.positionY = row[4].as<float>();
				r.info.positionZ = row[5].as<float>();
				r.info.orientation = row[6].as<float>();
				infos.push_back(r);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "LoadPlayerCreateInfo failed: " << e.what() << std::endl;
		}
		return infos;
	}

} // namespace MMO
