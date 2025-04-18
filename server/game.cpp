/* Start Header
*****************************************************************/
/*!
\file game.cpp
\author Poh Jing Seng, 2301363
\par jingseng.poh\@digipen.edu
\date 1 Apr 2025
\brief
This file implements the game handlers
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

#include "game.h"
#include <stdexcept>
#include <random>
#include <fstream>

#ifndef VERBOSE_LOGGING
#define VERBOSE_LOGGING
#endif

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
	auto last_asteroid_spawn_time = start;
	auto prev_frame_time = start;

	static bool prevGameRunning = gameRunning;
	prevGameRunning = gameRunning;

	while (gameRunning) {
		const auto curr = std::chrono::high_resolution_clock::now();
		const float dt = (float)(curr - prev_frame_time).count();

		// sleep to not lock Game::data permanently
		static constexpr int SLEEP_DURATION = 1000 / Server::TICK_RATE;
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION));
		static std::vector<char> dbytes;

		int num_spaceships{};
		int num_dead_spaceships{};

		// wont use a copy to avoid overwriting. 
		// will have to run fn on a separate timed thread
		{
			std::lock_guard<std::mutex> lock(data_mutex);

			num_spaceships = (int)data.spaceships.size();
			num_dead_spaceships = std::accumulate(
				data.spaceships.begin(),
				data.spaceships.end(),
				0,
				[](int n, const Spaceship& s) { return n + (s.lives_left ? 0 : 1); }
			);

			auto now = std::chrono::high_resolution_clock::now();
			const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - data.last_updated).count();

			// update spaceships
			for (Spaceship& s : data.spaceships) {
				s.pos += s.vector * dt;

				// Wrap spaceship positions 
				if (s.pos.x < 0) {
					s.pos.x = WINDOW_WIDTH;
				}
				else if (s.pos.x > WINDOW_WIDTH) {
					s.pos.x = 0;
				}

				if (s.pos.y < 0) {
					s.pos.y = WINDOW_HEIGHT;
				}
				else if (s.pos.y > WINDOW_HEIGHT) {
					s.pos.y = 0;
				}

			}

			// update bullets
			for (auto it = data.bullets.begin(); it != data.bullets.end();) {
				Bullet& b = *it;

				// remove bullets out of screen
				if (b.pos.x > WINDOW_WIDTH || b.pos.x < -WINDOW_WIDTH || b.pos.y > WINDOW_HEIGHT || b.pos.y < -WINDOW_HEIGHT) {
					it = data.bullets.erase(it);
					continue;
				}

				b.pos += b.vector * dt;
				++it;
			}

			// update asteroids
			for (Asteroid& a : data.asteroids) {
				a.pos += a.vector * dt;

				// wrap asteroid positions
				if (a.pos.x < -WINDOW_WIDTH) {
					a.pos.x = WINDOW_WIDTH;
				}
				else if (a.pos.x > WINDOW_WIDTH) {
					a.pos.x = -WINDOW_WIDTH;
				}

				if (a.pos.y < -WINDOW_HEIGHT) {
					a.pos.y = WINDOW_HEIGHT;
				}
				else if (a.pos.y > WINDOW_HEIGHT) {
					a.pos.y = -WINDOW_HEIGHT;
				}
			}

			if ((int)data.asteroids.size() < MAX_ASTEROIDS && std::chrono::duration<float, std::milli>(curr - last_asteroid_spawn_time).count() > ASTEROID_SPAWN_INTERVAL_MS) {
#ifdef VERBOSE_LOGGING
				{
					std::lock_guard<std::mutex> coutlock(Server::getInstance()._stdoutMutex);
					std::cout << "Spawning new asteroid" << std::endl;
				}
#endif

				// spawn asteroid
				last_asteroid_spawn_time = curr;

				static auto randomFloat = [](float min, float max) {
					static std::random_device rd;
					static std::mt19937 rng;
					std::uniform_real_distribution<float> dist(min, max);
					return dist(rng);
				};


				Asteroid na{};
				int edge = rand() % 4; // 0 = top, 1 = bottom, 2 = left, 3 = right

				if (edge == 0) { // Top edge
					na.pos = vec2(randomFloat(0, WINDOW_WIDTH), 0);
					na.vector = vec2(randomFloat(-1.f, 1.f), randomFloat(0.5f, 1.f)); // Move downward
				}
				else if (edge == 1) { // Bottom edge
					na.pos = vec2(randomFloat(0, WINDOW_WIDTH), WINDOW_HEIGHT);
					na.vector = vec2(randomFloat(-1.f, 1.f), randomFloat(-1.f, -0.5f)); // Move upward
				}
				else if (edge == 2) { // Left edge
					na.pos = vec2(0, randomFloat(0, WINDOW_HEIGHT));
					na.vector = vec2(randomFloat(0.5f, 1.f), randomFloat(-1.f, 1.f)); // Move right
				}
				else { // Right edge
					na.pos = vec2(WINDOW_WIDTH, randomFloat(0, WINDOW_HEIGHT));
					na.vector = vec2(randomFloat(-1.f, -0.5f), randomFloat(-1.f, 1.f)); // Move left
				}

				na.vector *= ASTEROID_SPEED;
				na.radius = (float)(rand() % (MAX_ASTEROID_RADIUS - MIN_ASTEROID_RADIUS) + MIN_ASTEROID_RADIUS);

				data.asteroids.push_back(na);
			}

			// check for collisions
			for (auto a_it = data.asteroids.begin(); a_it != data.asteroids.end();) {
				bool hasAsteroidCollided = false;

				// Check collision with bullets
				for (auto b_it = data.bullets.begin(); b_it != data.bullets.end();) {
					if (!circleCollision({ b_it->pos, b_it->radius }, { a_it->pos, a_it->radius })) {
						++b_it;
						continue;
					}

					auto owner = std::find_if(data.spaceships.begin(), data.spaceships.end(),
						[&b_it](const Spaceship& s) { return s.sid == b_it->sid; });

					if (owner != data.spaceships.end()) {
						owner->score += 1;
					}

					b_it = data.bullets.erase(b_it);
					hasAsteroidCollided = true;
					break;
				}

				if (hasAsteroidCollided) {
					a_it = data.asteroids.erase(a_it); // Erase safely, and continue iteration
					continue;
				}

				// Check collision with spaceships
				for (auto s_it = data.spaceships.begin(); s_it != data.spaceships.end(); ++s_it) {
					if (s_it->lives_left <= 0) continue;

					if (!circleCollision({ s_it->pos, s_it->radius }, { a_it->pos, a_it->radius })) {
						continue;
					}

#ifdef VERBOSE_LOGGING
					{
						std::lock_guard<std::mutex> coutlock(Server::getInstance()._stdoutMutex);
						std::cout << "Spaceship " << s_it->sid << " lost 1 life" << std::endl;
					}
#endif

					data.killSpaceship(s_it);
					hasAsteroidCollided = true;
					break;
				}

				if (hasAsteroidCollided) {
					a_it = data.asteroids.erase(a_it); // Erase safely
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
		std::vector<char> sbuf;
		sbuf.reserve(1 + dbytes.size()); // Avoids reallocations
		sbuf.push_back(Server::ALL_ENTITIES);
		sbuf.insert(sbuf.end(), dbytes.begin(), dbytes.end());

#ifdef JS_DEBUG
		std::vector<unsigned char> v(sbuf.begin(), sbuf.end());
#endif


		// broadcast data
		Server::getInstance().broadcastData(sbuf);

		// check elapsed time
		elapsed = std::chrono::high_resolution_clock::now() - start;

		if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > GAME_DURATION_S) {
			gameRunning = false;
			{
				std::lock_guard<std::mutex> lock(data_mutex);
			}
			{
				std::lock_guard<std::mutex> lock(Server::getInstance()._stdoutMutex);
				std::cout << "Time is up, ending game.." << std::endl;
			}
		}

		// check if all spaceships are dead(out of lives)
		if (num_dead_spaceships == num_spaceships) {
			gameRunning = false;
			{
				std::lock_guard<std::mutex> lock(data_mutex);
			}
			{
				std::lock_guard<std::mutex> lock(Server::getInstance()._stdoutMutex);
				std::cout << "All players are dead or disconnected, ending game.." << std::endl;
			}
		}
	}


	if (prevGameRunning && !gameRunning) {
		std::cout << "Game ended sending " << std::endl;
		// game ended, find winner sid
		// !NOTE: draw conditions not handled
		std::pair<int, int> winner_sid_score{ -1, -1 };
		{
			std::lock_guard<std::mutex> datalock(data_mutex);
			for (const auto& s : data.spaceships) {
				if (s.score > winner_sid_score.second) {
					winner_sid_score = { s.sid, s.score };
				}
			}

			// If no player has a positive score, assign a default winner
			if (winner_sid_score.first == -1 && !data.spaceships.empty()) {
				winner_sid_score.first = data.spaceships.front().sid;
			}
		}

		// get all time highest scores from file
		{
			std::ifstream ifs{ highscore_file };

			if (!ifs.is_open()) {
				//std::lock_guard<std::mutex> coutlock(Server::getInstance()._stdoutMutex);
				//std::cerr << "Cant open highscore file for reading " << highscore_file << std::endl;
			}

			for (int i{}; i < NUM_HIGHSCORES && !ifs.eof(); i++) {
				Highscore hs;
				if (!std::getline(ifs, hs.playername)) break;
				ifs >> hs.score;
				ifs.ignore();	// ignore newline
				std::getline(ifs, hs.datestring);

				highscores.push_back(hs);		// will be ordered from greatest to smallest
			}
		}

		// modify highscores if user made it onto all time high scores
		{
			for (auto it = highscores.begin(); it != highscores.end(); ++it) {
				if (winner_sid_score.second > it->score) {
					Highscore hs;
					{
						std::lock_guard<std::mutex> dlock(data_mutex);
						auto dsit = std::find_if(data.spaceships.begin(), data.spaceships.end(), [&winner_sid_score](const Spaceship& s) {
							return s.sid == winner_sid_score.first;
							}
						);
						if (dsit == data.spaceships.end()) {
							//std::cerr << "winner spaceship not found" << std::endl;
						}
						else {
							hs.playername = dsit->name;
						}
					}
					hs.score = winner_sid_score.second;
					hs.datestring = Server::getCurrentDateString();
					highscores.insert(it, hs);

					if (highscores.size() > NUM_HIGHSCORES) {
						highscores.pop_back();			// remove lowest score to maintain top 5 highscores
					}
					break;
				}
			}

			if (highscores.size() == 0) {
				Highscore hs;
				{
					std::lock_guard<std::mutex> dlock(data_mutex);
					auto dsit = std::find_if(data.spaceships.begin(), data.spaceships.end(), [&winner_sid_score](const Spaceship& s) {
						return s.sid == winner_sid_score.first;
						}
					);
					if (dsit == data.spaceships.end()) {
						//std::cerr << "winner spaceship not found" << std::endl;
					}
					else {
						hs.playername = dsit->name;
					}
				}
				hs.score = winner_sid_score.second;
				hs.datestring = Server::getCurrentDateString();
				highscores.push_back(hs);
			}
		}

		std::vector<char> ebuf;
		ebuf.push_back(Server::END_GAME);			// cmd
		ebuf.push_back(winner_sid_score.first);		// winner sid
		ebuf.push_back(winner_sid_score.second);	// winner score

		// populate highscores
		ebuf.push_back((char)highscores.size());		// num highscores

		for (int i{}; i < (int)highscores.size(); i++) {
			const Highscore& hs = highscores[i];
			ebuf.push_back((char)hs.score);				// highscore
			ebuf.push_back((char)hs.playername.size());	// playername length
			ebuf.insert(ebuf.end(), hs.playername.begin(), hs.playername.end());	// playername
		}


		auto reliable_bc = [this, ebuf]() {
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
					{
						std::lock_guard<std::mutex> aegclock(Server::getInstance().ack_end_game_clients_mutex);
						Server::getInstance().ack_end_game_clients.clear();
					}
					break;
				}

				Server::getInstance().broadcastData(ebuf);

				std::this_thread::sleep_for(std::chrono::milliseconds(Server::TIMEOUT_MS));

				elapsed = std::chrono::high_resolution_clock::now() - start_bc_time;
				if (elapsed.count() >= Server::DISCONNECTION_TIMEOUT_DURATION_MS / 1000.f) {
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

					{
						std::lock_guard<std::mutex> aegclock(Server::getInstance().ack_end_game_clients_mutex);
						Server::getInstance().ack_end_game_clients.clear();
					}
				}
			}

			{
				std::lock_guard<std::mutex> lock(data_mutex);
				data.reset();
			}
		};
		//std::thread rbc(reliable_bc);
		{
			std::lock_guard<std::mutex> tplock(Server::threadpool_mutex);
			Server::threadpool.push_back(std::async(std::launch::async, reliable_bc));
		}

		// update highscore file
		{
			std::ofstream ofs{ highscore_file };

			if (!ofs.is_open()) {
				std::lock_guard<std::mutex> coutlock(Server::getInstance()._stdoutMutex);
				std::cerr << "Cant open highscore file for writing " << highscore_file << std::endl;
			}

			for (const Highscore& hs : highscores) {
				ofs
					<< hs.playername << "\n"
					<< hs.score << "\n"
					<< hs.datestring << "\n";
			}
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

		bytes = Server::t_to_bytes(a.radius);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// asteroid radius (4 bytes)
	}

	return buf;
}

bool Game::circleCollision(Circle c1, Circle c2) {
	const float rsq = powf(c1.radius + c2.radius, 2.f);
	const float dsq = (c2.pos - c1.pos).lengthSq();

	return dsq <= rsq;
}

void Game::Data::reset() {
	bullets.clear();
	asteroids.clear();
	last_updated = std::chrono::high_resolution_clock::now();

	for (Spaceship& s : spaceships) {
		s.pos = { WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f };
		s.vector = { 0,0 };
		s.rotation = 3.3f;
		s.lives_left = Game::NUM_START_LIVES;
		s.score = 0;
	}
}

void Game::Data::killSpaceship(std::vector<Spaceship>::iterator& it) {
	it->pos = { WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f };
	it->vector = { 0,0 };
	it->rotation = 0.f;
	it->lives_left--;
}
