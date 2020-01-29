#include "Arduino.h"

// Define the bit patters for each of our custom chars. These are 5 bits wide and 8 dots deep
uint8_t custChar[8][8] = {
							//
							{ 31, 31, 31, 0, 0, 0, 0, 0 },      // Small top line - 0
							{ 0, 0, 0, 0, 0, 31, 31, 31 },      // Small bottom line - 1
							// This shows an alternative way of defining custom characters, a bit more visually
							{
							B11111,
							B00000,
							B00000,
							B00000,
							B00000,
							B00000,
							B00000,
							B11111, },
							//{31, 0, 0, 0, 0, 0, 0, 31},		// Small lines top and bottom -2
							{ 0, 0, 0, 0, 0, 0, 0, 31 },		// Thin bottom line - 3
							{ 31, 31, 31, 31, 31, 31, 15, 7 },	// Left bottom chamfer full - 4
							{ 28, 30, 31, 31, 31, 31, 31, 31 },	// Right top chamfer full -5
							{ 31, 31, 31, 31, 31, 31, 30, 28 },	// Right bottom chamfer full -6
							{ 7, 15, 31, 31, 31, 31, 31, 31 },	// Left top chamfer full -7
};

// Define our numbers 0 thru 9 plus minus symbol (can't use any more custom chars above, boo)
// 254 is blank and 255 is the "Full Block"
uint8_t bigNums[10][6] = {
	//
	{ 7, 0, 5, 4, 1, 6 },         //0
	{ 0, 5, 254, 1, 255, 1 },     //1
	{ 0, 2, 5, 7, 3, 1 },         //2
	{ 0, 2, 5, 1, 3, 6 },         //3
	{ 7, 3, 255, 254, 254, 255 }, //4
	{ 7, 2, 0, 1, 3, 6 },         //5
	{ 7, 2, 0, 4, 3, 6 },         //6
	{ 0, 0, 5, 254, 7, 254 },     //7
	{ 7, 2, 5, 4, 3, 6 },         //8
	{ 7, 2, 5, 1, 3, 6 },         //9
};

// Generic array size fetcher
template<typename T, unsigned S>
inline unsigned arraysize(const T (&v)[S]) {
	return S;
}
