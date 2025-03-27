# i think can just print this to pdf through browser for design doc?

Note: all vectors will not be normalized (normalized vector * velocity) to save bytes passed

Server determines:
- new client spaceship spawn point
- new asteroid spawn, vector
- highest player score
- player ranking (only at end of the game)

Client determines:
- asteroid destruction (collision)
- self movement/rotation
- new bullet creation (vector)


```cpp
enum CLIENT_REQUESTS {
  CONN_REQUEST = 0,
  ACK_CONN_REQUEST,
  REQ_START_GAME,
  ACK_START_GAME,
  ACK_ACK_SELF_SPACESHIP,
  SELF_SPACESHIP,
  ACK_ALL_ENTITIES,
  ACK_END_GAME
};

enum SERVER_MSGS {
  CONN_ACCEPTED = 0,
  CONN_REJECTED,
  START_GAME,
  ACK_SELF_SPACESHIP,
  ALL_ENTITIES,
  END_GAME
}
```

Floating point numbers

<details>
<summary>Manual method</summary>
float structure
1. 1 bit sign - 0 positive, 1 negative
2. 7 bits exponent
3. 3 bytes mantissa

Convert to binary (using 5.75)
1. 5 -> 0101
2. 0.75 -> 1100
3. 5.75 -> 101.11

(sign bit) mantissa * 2^exponent

4. sign bit = 0
5. exponent = 2 (.11 is length 2)
6. mantissa = 1.0111 (shift 2 bits to the right)

6. mantissa = 5.75 / 2^2 = 1.4375

Representation = + 1.4375 * 2^2

Hence,
- sign = 0 
- exponent = 2 + 63(bias for 7 bit) = 65
- mantissa = `0b0111`

7. Convert to binary

0 1000001 011100000000000000000000
</details>
<details>
<summary>C++ bitset method</summary>

### Float to binary
```cpp
float num = 5.75;
unsigned int* int_ptr = reinterpret_cast<unsigned int*>(&num);
std::bitset<32> binary(*int_ptr);   // iterable
```

### Binary to float 
```cpp
constexpr const char* binary_str = "01000001011100000000000000000000";  // dont quote me idk if i typed the right number of 0s lol
std::bitset<32> binary(binary_str);
unsigned int int_rep = static_cast<unsigned int>(binary.to_ulong());
float num = *reinterpret_cast<float*>(&int_rep);
```


</details>


# Flow

## Client requests for a game

Client request:
- `CONN_REQUEST` [1 byte]

Server response:
- `CONN_ACCEPTED`/`CONN_REJECTED` [1 byte]
- session ID [1 byte]
- spawn location in xy coords [8 bytes signed] - use float with mantissa/exponent
- spawn rotation in degrees [2 bytes int]
- num lives [1 byte]

Client response:
- `ACK_CONN_REQUEST`, 1 byte. for both `CONN_ACCEPTED` and `CONN_REJECTED`


## Start game

1. Any client sends `REQ_START_GAME` request to server
2. Server sends `START_GAME` response to all clients
3. Client sends `ACK_START_GAME` and starts game (allow players to move and shoot)
4. Server receives `ACK_START_GAME`, else times out and send requests again. If no response if 5 seconds, disconnects client

## On every frame

1. Client sends self spaceship details to server

- `SELF_SPACESHIP`
  - 1 byte
- session ID
  1 byte
- Position xy coords 
  - 4 bytes (2 bytes each)
- Vector
  - 8 bytes (4 bytes each)
- Rotation (unsigned in degrees)
  - 2 bytes
- Lives left


- Num new bullets fired by this spaceship in this frame
  - 1 byte

for each bullet,
{
  - Position xy coords for each bullet (4 bytes)
  - each bullet vector (8 bytes)
  - ~~each bullet rotation (2 bytes)~~ (using circle bullet, no need for rotation)
}

- Num asteroids destroyed this frame
  - 1 byte

for each asteroid
{
  - asteroid ID (4 bytes)
}

###### --


2. Server updates location
  - If packet is received, overwrite position and send `ACK_SELF_SPACESHIP`. client has to respond withh `ACK_ACK_SELF_SPACESHIP`
  - Else, calculate new position based on previous position and vector

3. Server updates asteroids
  - remove asteroids destroyed

4. Server updates bullets
  - remove dead bullets (out of frame, since this game uses a fixed camera)

5. Server sends entities data to clients
  - `ALL_ENTITIES`    // use as rendering data?
    - 1 byte
  - Number of spaceships (alive, dead spaceships dont have to be rendered)
    - 1 byte

    for each spaceship
    {
      - Position xy coords 
        - 4 bytes (2 bytes each)
      - Vector
        - 8 bytes (4 bytes each)
      - Rotation (unsigned in degrees)
        - 2 bytes
      - Lives left
    }
  - Number of bullets
    - 1 byte

    for each bullet
    {
      - Position xy coords for each bullet (4 bytes)
      - each bullet vector (8 bytes)
    }

  - Number of asteroids (alive)
    - 1 byte
  
    for each asteroid (new asteroids will be added in here when spawned)
    {
      - asteroid ID (4 bytes)
      - Position xy coords (4 bytes, 2 bytes each)
      - Vector (8 bytes, 4 bytes each)
    }


6. Client receives `ALL_ENTITIES` and renders frame

## End game

1. Server ends game when `GAME_DURATION` is reached or when all spaceships are dead
2. Server sends `END_GAME`, 1 byte to clients
3. Clients send `ACK_END_GAME` and displays end game screen



# Others

1. Client will be timed out in `DISCONNECTION_TIMEOUT`, when that happens, client's spaceship will be removed

