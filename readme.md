# Payloads

## CONN_REQUEST [CLIENT]
```cpp
cmd - 1 byte
player name length - 1 byte
player name - n bytes
```

## CONN_REJECTED [SERVER RELIABLE]
```cpp
cmd - 1 byte
```

## CONN_ACCEPTED [SERVER RELIABLE]
```cpp
cmd - 1 byte
session id - 1 byte
spawn pos x - 4 bytes [float]
spawn pos y - 4 bytes [float]
spawn rotation degrees - 4 bytes [float]
```

## ACK_CONN_REQUEST [CLIENT]
```cpp
cmd - 1 byte
session id - 1 byte
```

## REQ_START_GAME [CLIENT]
```cpp
cmd - 1 byte
```

## START_GAME [SERVER RELIABLE]
```cpp
cmd - 1 byte
num players - 1 byte

// for n players
sid - 1 byte
player name length -  1 byte
player name - n bytes
```

## ACK_START_GAME [CLIENT]
```cpp
cmd - 1 byte
session id - 1 byte
```

## SELF_SPACESHIP [CLIENT]
```cpp
cmd - 1 byte
session id - 1 byte

vector x - 4 bytes [float]
vector y - 4 bytes [float]
rotation - 4 bytes [float]
```


## NEW_BULLET [CLIENT RELIABLE]
```cpp
cmd - 1 byte
session id - 1 byte

seq number - 4 bytes		// also used as bullet id

pos x - 4 bytes
pos y - 4 bytes
vector x - 4 bytes
vector y - 4 bytes
```



~~num new asteroids destroyed - 1 byte (gotta keep track, if server does not ack, store data and send again next frame)~~

~~asteroid id - 4 bytes~~

## ACK_NEW_BULLET [SERVER]
```cpp
cmd - 1 byte
seq number - 4 bytes
```

<details>
<summary>removed</summary>

## ACK_ACK_SELF_SPACESHIP [CLIENT]
used to tell server that client has received ack for payload, as client stores data that was not received by server.
eg. bullet fired this frame, not received by server, hence resend this bullet next frame
```cpp
cmd - 1 byte
session id - 1 byte
```

## ACK_ALL_ENTITIES [CLIENT] 
```cpp
cmd - 1 byte
session id - 1 byte
```

</details>

## ALL_ENTITIES [SERVER RELIABLE] (Client to start rendering upon receiving)
asteroids will be sent from here for spawning
```cpp
cmd - 1 byte

time left in seconds - 1 byte

num active spaceships - 1 byte

// for n spaceships
session id spaceship belongs to - 1 byte
pos x - 4 bytes [float]
pos y - 4 bytes[float]
rotation - 4 bytes [float]
lives left - 1 byte
score - 1 byte

// for n bullets
session id bullet belongs to - 1 byte
pos x - 4 bytes[float]
pos y - 4 bytes[float]

// for n asteroids
pos x - 4 bytes [float]
pos y - 4 bytes [float]
```

## END_GAME [SERVER RELIABLE]
```cpp
cmd - 1 byte
winner session id - 1 byte
winner score - 1 byte

num highscores - 1 byte

// for top n of all time high scores
all time high score - 1 byte
playername length - 1 byte
playername - n bytes

date string length = 1 byte
date string - n bytes
```

## ACK_END_GAME [CLIENT]
```
cmd - 1 byte
session id - 1 byte
```

## KEEP_ALIVE [CLIENT]
```
cmd - 1 byte
session id - 1 byte
```

## 


# Flow

1. Client inits connection with `CONN_REQUEST`
  - Server responds with `CONN_ACCEPTED` or `CONN_REJECTED`
  -  Client sends `ACK_CONN_REQUEST`
  - at this point, if rejected, client closes
2. Client sends `REQ_START_GAME` (on user input)
  - Server broadcasts `START_GAME`
  - Client responds with `ACK_START_GAME`
3. On every user input, client handles the vector(and velocity) and rotation change, and sends to server with command `SELF_SPACESHIP`
  - Server will respond with `ACK_SELF_SPACESHIP` (not broadcast)
4. On every tick, server will update and broadcast the positional data of all entities to client for rendering with command `ALL_ENTITIES`
  - Client will respond with `ACK_ALL_ENTITIES`
5. On game end(either time based or when all spaceships die), server will broadcast `END_GAME`
  - Client will respond with `ACK_END_GAME` and display winner with highest score



# Info

Note: all vectors will not be normalized (normalized vector * velocity) to save bytes passed


```cpp
	enum CLIENT_REQUESTS {
		CONN_REQUEST = 0,
		ACK_CONN_REQUEST,
		REQ_START_GAME,
		ACK_START_GAME,
		SELF_SPACESHIP,
		ACK_ALL_ENTITIES,
		ACK_END_GAME,
		KEEP_ALIVE
	};

	enum SERVER_MSGS {
		CONN_ACCEPTED = 0,
		CONN_REJECTED,
		START_GAME,
    ACK_SELF_SPACESHIP,
		ALL_ENTITIES,
		END_GAME,
	};
```

### Float to binary and vice versa
```cpp
	template <typename T>
	std::vector<char> t_to_bytes(T num) {
		std::vector<char> bytes(sizeof(T));

		if constexpr (std::is_integral<T>::value) {
			// for ints, convert to network byte order
			num = htonl(num);
			std::memcpy(bytes.data(), &num, sizeof(T));
		}
		else if constexpr (std::is_floating_point<T>::value) {
			// For floats
			unsigned int int_rep = *reinterpret_cast<unsigned int*>(&num);  // reinterpret float as unsigned int
			int_rep = htonl(int_rep);
			std::memcpy(bytes.data(), &int_rep, sizeof(T));
		}

		return bytes;
	}

	float btof(const std::vector<char>& bytes) {
		if (bytes.size() != sizeof(float)) {
			throw std::invalid_argument("Invalid byte size for float conversion");
		}

		unsigned int int_rep;
		std::memcpy(&int_rep, bytes.data(), sizeof(float));
		int_rep = ntohl(int_rep);  // Convert from network byte order

		float num;
		std::memcpy(&num, &int_rep, sizeof(float));
		return num;
	}
```


</details>


