#include "game.h"
#include <stdexcept>

Game& Game::getInstance() {
	static Game game;
	return game;
}

/**
 * locks Game::data variable for the whole function call.
 * 
 */
void Game::updateGame() {
	// make a copy of Game data to avoid locking resource for too long
	//Data data_copy;
	//{
	//	std::lock_guard<std::mutex> lock(data_mutex);
	//	data_copy = data;
	//}

	std::chrono::duration<float> elapsed{};
	auto start = std::chrono::high_resolution_clock::now();

	static bool prevGameRunning = gameRunning;
	prevGameRunning = gameRunning;

	while (gameRunning) {
		// sleep to not lock Game::data permanently
		static constexpr int SLEEP_DURATION = 1000 / Server::TICK_RATE;
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION));
		static std::vector<char> dbytes;

		// wont use a copy to avoid overwriting. 
		// will have to run fn on a separate timed thread
		{
			std::lock_guard<std::mutex> lock(data_mutex);

			auto now = std::chrono::high_resolution_clock::now();
			const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - data.last_updated).count();

			// update spaceships
			for (Spaceship& s : data.spaceships) {
				s.pos += s.vector;
			}

			// update bullets
			for (Bullet& b : data.bullets) {
				b.pos += b.vector;
			}

			// update asteroids
			for (Asteroid& a : data.asteroids) {
				a.pos += a.vector;
			}

			// check for collisions
			for (auto a_it = data.asteroids.begin(); a_it != data.asteroids.end();) {
				const Asteroid& a = *a_it;
				bool hasAsteroidCollided = false;

				// check for collision with bullet
				for (auto b_it = data.bullets.begin(); b_it != data.bullets.end();) {
					const Bullet& b = *b_it;

					if (!circleCollision({ b.pos, b.radius }, { a.pos, a.radius })) {
						++b_it;
						continue;
					}
					auto owner = std::find_if(data.spaceships.begin(), data.spaceships.end(), [&b](const Spaceship& s) {return s.sid == b.sid; });

					if (owner == data.spaceships.end()) {
						constexpr const char* err = "Bullet owner(spaceship) does not exist";
						std::cerr << err << std::endl;
						throw std::runtime_error(err);
					}

					++owner->score;
					b_it = data.bullets.erase(b_it);
					hasAsteroidCollided = true;
					break;
				}

				if (hasAsteroidCollided) {
					a_it = data.asteroids.erase(a_it);
					continue;
				}

				// check for collision with spaceship
				for (auto s_it = data.spaceships.begin(); s_it != data.spaceships.end();) {
					const Spaceship& s = *s_it;

					if (!circleCollision({ s.pos, s.radius }, { a.pos, a.radius })) {
						++s_it;
						continue;
					}

					data.killSpaceship(s_it);
					// !NOTE: wont be removed as need to keep track of score, client to handle dead spaceships
					//if (s_it->lives_left <= 0) {
					//	s_it = data.spaceships.erase(s_it);
					//}
					hasAsteroidCollided = true;
					break;
				}

				if (hasAsteroidCollided) {
					a_it = data.asteroids.erase(a_it);
					continue;
				}
				else {
					++a_it;
				}
			}


			// update last updated time
			data.last_updated = std::chrono::high_resolution_clock::now();

			// update sending buffer
			dbytes = data.toBytes();
		}

		// data update is done, send to clients

		// prepare payload
		static std::vector<char> sbuf(Server::MAX_PACKET_SIZE);
		sbuf.push_back(Server::ALL_ENTITIES);
		sbuf.insert(sbuf.end(), dbytes.begin(), dbytes.end());

		// broadcast data
		Server::getInstance().broadcastData(sbuf);

		// check elapsed time
		elapsed = std::chrono::high_resolution_clock::now() - start;

		if (elapsed.count() > GAME_DURATION_S * 1000) {
			gameRunning = false;
		}
	}


	if (prevGameRunning && !gameRunning) {
		// game ended, find winner sid
		// !NOTE: draw conditions not handled
		std::pair<int, int> winner_sid_score{ -1, -1 };
		{
			std::lock_guard<std::mutex> datalock(data_mutex);
			std::for_each(data.spaceships.begin(), data.spaceships.end(), [&winner_sid_score](const Spaceship& s) {
				if (s.score > winner_sid_score.second) {
					winner_sid_score.first = s.sid;
					winner_sid_score.second = s.score;
				}
				}
			);
		}

		std::vector<char> ebuf;
		ebuf.push_back(Server::END_GAME);
		ebuf.push_back(winner_sid_score.first);

		auto reliable_bc = [this, &ebuf]() {
			std::chrono::duration<float> elapsed;
			auto start_bc_time = std::chrono::high_resolution_clock::now();

			while (true) {
				int num_acked{};
				{
					std::lock_guard<std::mutex> aegclock(Server::getInstance().ack_end_game_clients_mutex);
					num_acked = (int)Server::getInstance().ack_end_game_clients.size();
				}

				int expected_acks{};
				{
					std::lock_guard<std::mutex> datalock(data_mutex);
					expected_acks = (int)data.spaceships.size();
				}

				if (num_acked == expected_acks) {
					std::lock_guard<std::mutex> stdoutLock(Server::getInstance()._stdoutMutex);
					std::cout << "All clients ACKed END_GAME command" << std::endl;
					break;
				}

				Server::getInstance().broadcastData(ebuf);

				std::this_thread::sleep_for(std::chrono::milliseconds(Server::TIMEOUT_MS));

				elapsed = std::chrono::high_resolution_clock::now() - start_bc_time;
				if (elapsed.count() > Server::DISCONNECTION_TIMEOUT_DURATION_MS) {
					// disconnect clients that did not ack
					{
						std::lock_guard<std::mutex> stdoutLock(Server::getInstance()._stdoutMutex);
						std::cout << num_acked << "/" << expected_acks << " ACKed END_GAME command. Disconnecting timed out clients." << std::endl;
					}

					std::unordered_set<SESSION_ID> acked;
					{
						std::lock_guard<std::mutex> aegclock(Server::getInstance().ack_end_game_clients_mutex);
						acked = Server::getInstance().ack_end_game_clients;
					}

					{
						std::lock_guard<std::mutex> datalock2(data_mutex);
						for (auto it = data.spaceships.begin(); it != data.spaceships.end();) {
							if (acked.find(it->sid) != acked.end()) {
								++it;
								continue;
							}

							// spaceship(client) did not ack, remove
							it = data.spaceships.erase(it);
						}
					}
				}
			}
			};
		reliable_bc();

		{
			std::lock_guard<std::mutex> lock(data_mutex);
			data.reset();
		}

		prevGameRunning = gameRunning;
	}
}


std::vector<char> Game::Data::toBytes() {
	std::vector<char> buf;

	// num spaceships
	buf.push_back((char)spaceships.size());
	std::vector<char> bytes;

	for (const Spaceship& s : spaceships) {
		// only stuff integral to rendering/interaction is sent
		// vector is not sent

		buf.push_back(s.sid & 0xff);		// spaceship sid

		bytes = Server::t_to_bytes(s.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(s.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)

		bytes = Server::t_to_bytes(s.rotation);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// rotation in degrees (4 bytes)

		buf.push_back(s.lives_left);						// lives left (1 byte)

		buf.push_back(s.score);								// score (1 byte)
	}

	// num bullets
	buf.push_back((char)bullets.size());

	for (const Bullet& b : bullets) {
		// vector is not sent, not integral to rendering

		buf.push_back(b.sid & 0xff);		// bullet sid

		bytes = Server::t_to_bytes(b.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(b.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	// num asteroids
	buf.push_back((char)asteroids.size());

	for (const Asteroid& a : asteroids) {
		// vector not sent, not needed for rendering

		bytes = Server::t_to_bytes(a.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = Server::t_to_bytes(a.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	return buf;
}

bool Game::circleCollision(Circle c1, Circle c2) {
	const float rsq = powf(max(c1.radius, c2.radius), 2.f);
	const float lsq = (c2.pos - c1.pos).lengthSq();
	
	return lsq >= rsq;
}

void Game::Data::reset() {
	bullets.clear();
	asteroids.clear();
	last_updated = std::chrono::high_resolution_clock::now();

	for (Spaceship& s : spaceships) {
		s.pos = { 0, 0 };
		s.vector = { 0,0 };
		s.rotation = 0.f;
		s.lives_left = Game::NUM_START_LIVES;
		s.score = 0;
	}
}

void Game::Data::killSpaceship(std::vector<Spaceship>::iterator& it) {
	it->pos = { 0, 0 };
	it->vector = { 0,0 };
	it->rotation = 0.f;
	it->lives_left--;
}
