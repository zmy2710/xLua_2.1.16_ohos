#include "lua.h"
#include "lauxlib.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

static union {char c[4]; unsigned long mylong;} endian_test ={'l','?','?','b'};
#define PLATFORM_ENDIANNESS ((char)endian_test.mylong)

#define BITS_PER_WORD (CHAR_BIT*sizeof(unsigned int))
#define I_WORD(i) ((unsigned int)(i)/BITS_PER_WORD)
#define I_BIT(i) (1 << ((unsigned int)(i) % BITS_PER_WORD))

typedef unsigned char byte;
typedef int bool;
#define true 1
#define false 0

union IntFloat
{
	int intvalue;
	float floatvalue;
};
typedef union IntFloat intfloat;

union Int64Double
{
	int64_t int64value;
	double doublevalue;
};
typedef union Int64Double int64double;

//默认设置成小端
static char Target_Endianness = 'l';
static char Bigendian = 'b';
static char Littleendian = 'l';

/*
typedef struct NumArray
{
    int size;
    unsigned int values[1];
}NumArray;

//创建一个数组
static int l_newarray(lua_State *L)
{
    int i, n;
    size_t nbytes;
    NumArray *a;

    n = luaL_checkint(L, 1);
    luaL_argcheck(L, n >= 1, 1, "invalid size");
    nbytes = sizeof(NumArray) + I_WORD(n - 1)*sizeof(unsigned int);
    a = (NumArray *)lua_newuserdata(L, nbytes);

    a->size = n;
    for(i = 0;i <= I_WORD(n-1);i++)
        a->values[i] = 0;// 初始化数组
    return 1;//新的userdata已经在栈上
}

//设置数组内容
static int l_setarray(lua_State *L)
{
    NumArray *a = (NumArray*)lua_touserdata(L, 1);
    int index = luaL_checkint(L, 2) - 1;
    luaL_checkany(L, 3);
    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    luaL_argcheck(L, 0 <= index && index < a->size, 2, "index out of range");

    if(lua_toboolean(L, 3))
        a->values[I_WORD(index)] |= I_BIT(index);
    else
        a->values[I_WORD(index)] &= ~I_BIT(index);
}

//获取数组内容
static int l_getarray(lua_State *L)
{
    NumArray *a = (NumArray*)lua_touserdata(L, 1);
    int index = luaL_checkint(L, 2) - 1;
    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    luaL_argcheck(L, 0 <= index && index < a->size, 2, "index out of range");
   
    lua_pushboolean(L, a->values[I_WORD(index)] & I_BIT(index));
    return 1;
}

//获取数组大小
static int l_getsize(lua_State *L)
{
    NumArray *a = (NumArray*)lua_touserdata(L, 1);
    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    lua_pushinteger(L, a->size);
    return 1;
}


*/
static int l_haode(lua_State *L)
{
    lua_pushnumber(L, 3);
    return 1;
}

static int l_readbytearray(lua_State *L)
{
	byte* const_ptr = (byte*)luaL_checkinteger(L, 1);
	byte* ptr = const_ptr;
	for (int i = 0; i < 30; i++)
	{
		lua_pushinteger(L, *(ptr + i));
	}
	return 30;
}

// 检查是否越界
static bool AssetOutOfRange(lua_State *L, int startpos, int datalen, int maxsize)
{
	if(startpos < 0 || startpos + datalen >= maxsize)
	{
		luaL_error(L, "Index out of range.");
		//把wpos搞到最后，让后都没法写，避免中间没写，后面小值写成功的问题
		lua_pushinteger(L, maxsize - 1);
		return true;
	}
	else
	{
		return false;
	}
}

//---------内部读写buffer函数---------
//  
//------------------------------------

//往指定地址写入一个unsigned char
static void WriteByte(byte* pos, byte value)
{
	*pos = value;
}

//从一个地址开始，反序拷贝字节流到一个数组，为了处理大小端
static void CopyArrayReverse(char* sourcePtr, char* dstArray, int dstArrayLen)
{
	for(int i = 0;i < dstArrayLen;i++)
	{
		dstArray[dstArrayLen - i - 1] = *(sourcePtr + i);
	}
}




//---------使用前需要先调用接口设置字节流的大小端属性，所有的读写接口都会以设置的端性进行处理----------
//  参数 memInst, rpos, maxsize
//  返回 rpos, value
//------------------------------------

//设置字节流的端性，0是小端，1是大端
static int l_setEndianness(lua_State *L)
{
	int endianness = luaL_checkinteger(L, 1);
	if(endianness > 0)
	{
		Target_Endianness = Bigendian;
	}
	else
	{
		Target_Endianness = Littleendian;
	}
}

static int l_readInt8(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 1, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 1);
	lua_pushinteger(L, *(int8_t *)(ptr + rpos));
	return 2;
}

static int l_readInt16(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 2, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 2);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(int16_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[2];
		CopyArrayReverse(ptr + rpos, &buffer[0], 2);
		lua_pushinteger(L, *(int16_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readInt32(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 4, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 4);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(int32_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[4];
		CopyArrayReverse(ptr + rpos, &buffer[0], 4);
		lua_pushinteger(L, *(int32_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readInt64(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 8, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 8);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(int64_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[8];
		CopyArrayReverse(ptr + rpos, &buffer[0], 8);
		lua_pushinteger(L, *(int64_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readUInt8(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 1, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 1);
	lua_pushinteger(L, *(uint8_t *)(ptr + rpos));
	return 2;
}

static int l_readUInt16(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 2, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 2);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(uint16_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[2];
		CopyArrayReverse(ptr + rpos, &buffer[0], 2);
		lua_pushinteger(L, *(uint16_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readUInt32(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 4, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 4);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(uint32_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[4];
		CopyArrayReverse(ptr + rpos, &buffer[0], 4);
		lua_pushinteger(L, *(uint32_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readUInt64(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 8, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 8);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushinteger(L, *(uint64_t *)(ptr + rpos));
	}
	else
	{
		byte buffer[8];
		CopyArrayReverse(ptr + rpos, &buffer[0], 8);
		lua_pushinteger(L, *(uint64_t *)(&buffer[0]));
	}
	return 2;
}

static int l_readFloat(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 4, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 4);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushnumber(L, *(float *)(ptr + rpos));
	}
	else
	{
		byte buffer[4];
		CopyArrayReverse(ptr + rpos, &buffer[0], 4);
		lua_pushnumber(L, *(float *)(&buffer[0]));
	}
	return 2;
}

static int l_readDouble(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, rpos, 4, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + 8);
	if(PLATFORM_ENDIANNESS == Target_Endianness)
	{
		lua_pushnumber(L, *(double *)(ptr + rpos));
	}
	else
	{
		byte buffer[8];
		CopyArrayReverse(ptr + rpos, &buffer[0], 8);
		lua_pushnumber(L, *(double *)(&buffer[0]));
	}
	return 2;
}

static int l_readString(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	const char* strptr = ptr + rpos;
	int strlength = strlen(strptr);
	if (AssetOutOfRange(L, rpos, strlen(ptr), maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + strlength + 1);
	lua_pushstring(L, strptr);
	return 2;
}

//这个接口一般不会在lua调用，因为只能返回string，如果后续有需要，可以自定义一个userdata用一下。
static int l_readBlob(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int rpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	uint8_t bloblen = *(uint8_t *)ptr;
	const char* strptr = ptr + rpos + 4;
	if (AssetOutOfRange(L, rpos, bloblen, maxsize) == true)
	{
		lua_pushinteger(L, 0);
		return 2;
	}
	lua_pushinteger(L, rpos + bloblen + 4);
	lua_pushlstring(L, strptr, bloblen);
	return 2;
}

//---------写c#中的bytearray，根据设置的大小端属性处理----------
//  参数 memInst, wpos, maxsize, value
//  返回 wpos
//------------------------------------
static int l_writeInt8(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 1, maxsize) == true)
	{
		return 1;
	}
	int8_t value = luaL_checkinteger(L, 4);
	WriteByte(ptr + wpos, value);
	lua_pushinteger(L, ++wpos);
	return 1;
}

static int l_writeInt16(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 2, maxsize) == true)
	{
		return 1;
	}
	int16_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		WriteByte(ptr + wpos++, value & 0xff);
		WriteByte(ptr + wpos++, value >> 8 & 0xff);
	}
	else
	{
		WriteByte(ptr + wpos++, value >> 8 & 0xff);
		WriteByte(ptr + wpos++, value & 0xff);
	}
	lua_pushinteger(L, wpos);
	return 1;
}

static int l_writeInt32(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 4, maxsize) == true)
	{
		return 1;
	}
	int32_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i,(value >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i,(value >> (4 - i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 4);
	return 1;
}

static int l_writeInt64(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 8, maxsize) == true)
	{
		return 1;
	}
	int64_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i,(value >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i,(value >> (8 - i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 8);
	return 1;
}

static int l_writeUInt8(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 1, maxsize) == true)
	{
		return 1;
	}
	uint8_t value = luaL_checkinteger(L, 4);
	WriteByte(ptr + wpos, value);
	lua_pushinteger(L, ++wpos);
	return 1;
}

static int l_writeUInt16(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 2, maxsize) == true)
	{
		return 1;
	}
	uint16_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		WriteByte(ptr + wpos++, value & 0xff);
		WriteByte(ptr + wpos++, value >> 8 & 0xff);
	}
	else
	{
		WriteByte(ptr + wpos++, value >> 8 & 0xff);
		WriteByte(ptr + wpos++, value & 0xff);
	}
	lua_pushinteger(L, wpos);
	return 1;
}

static int l_writeUInt32(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 4, maxsize) == true)
	{
		return 1;
	}
	uint32_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i, (value >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i, (value >> ( 4- i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 4);
	return 1;
}

//lua53的64位只能支持int64值的全覆盖，没办法实现uint64的全覆盖。我们实际上只能支持uin63值的全覆盖，也就是说符号位是比较特殊的。
static int l_writeUInt64(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 8, maxsize) == true)
	{
		return 1;
	}
	uint64_t value = luaL_checkinteger(L, 4);
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i, (value >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i, (value >> (8 - i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 8);
	return 1;
}

static int l_writeFloat(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 4, maxsize) == true)
	{
		return 1;
	}
	float value = luaL_checknumber(L, 4);
	intfloat uvalue;
	uvalue.floatvalue = value;
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i, (uvalue.intvalue >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 4; i++)
			WriteByte(ptr + wpos + i, (uvalue.intvalue >> ( 4 - i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 4);
	return 1;
}

static int l_writeDouble(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);
	if (AssetOutOfRange(L, wpos, 4, maxsize) == true)
	{
		return 1;
	}
	double value = luaL_checknumber(L, 4);
	int64double uvalue;
	uvalue.doublevalue = value;
	if(Target_Endianness == Littleendian)
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i, (uvalue.int64value >> i * 8 & 0xff));
	}
	else
	{
		for (int i = 0; i < 8; i++)
			WriteByte(ptr + wpos + i, (uvalue.int64value >> (8 - i - 1) * 8 & 0xff));
	}
	lua_pushinteger(L, wpos + 8);
	return 1;
}

static int l_writeString(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);

	const char* strptr = luaL_checkstring(L, 4);
	int strlength = strlen(strptr);
	if (AssetOutOfRange(L, wpos, strlength, maxsize) == true)
	{
		return 1;
	}
	byte* startPtr = ptr + wpos;
	for (int i = 0; i < strlength; i++)
		WriteByte(startPtr++, *(strptr + i));
	WriteByte(startPtr, '\0');
	lua_pushinteger(L, wpos + strlength + 1);
	return 1;
}

static int l_writeBlob(lua_State *L)
{
	byte* ptr = (byte*)luaL_checkinteger(L, 1);
	int wpos = luaL_checkinteger(L, 2);
	int maxsize = luaL_checkinteger(L, 3);

	const char* strptr = luaL_checkstring(L, 4);
	int strlength = strlen(strptr);
	if (AssetOutOfRange(L, wpos, strlength, maxsize) == true)
	{
		return 1;
	}
	//先把长度写进去，以uint32形式
	int len = strlength;
	byte* startPtr = ptr + wpos;
	for (int i = 0; i < 4; i++)
		WriteByte(startPtr++, (len >> i * 8 & 0xff));
	//再把内容写进去
	for (int i = 0; i < strlength; i++)
		WriteByte(startPtr++, *(strptr + i));
	lua_pushinteger(L, wpos + strlength + 4);
	return 1;
}

/*

public void writeRawStream(byte[] v)
{
	int size = v.Length;
	if (size > space())
	{
		Dbg.ERROR_MSG("memorystream::writeRawStream: no free!");
		return;
	}

	for (int i = 0; i < size; i++)
	{
		datas_[wpos++] = v[i];
	}
}



public void writeVector2(Vector2 v)
{
	writeFloat(v.x);
	writeFloat(v.y);
}

public void writeVector3(Vector3 v)
{
	writeFloat(v.x);
	writeFloat(v.y);
	writeFloat(v.z);
}

public void writeVector4(Vector4 v)
{
	writeFloat(v.x);
	writeFloat(v.y);
	writeFloat(v.z);
	writeFloat(v.w);
}

public void writeEntitycall(byte[] v)
{
	UInt64 cid = 0;
	Int32 id = 0;
	UInt16 type = 0;
	UInt16 utype = 0;

	writeUint64(cid);
	writeInt32(id);
	writeUint16(type);
	writeUint16(utype);
}
*/





static const struct luaL_Reg memstream [] = 
{
#define ENTRY(name) {#name, l_##name}
	ENTRY(setEndianness),
	ENTRY(writeInt8),
	ENTRY(writeInt16),
	ENTRY(writeInt32),
	ENTRY(writeInt64),
	ENTRY(writeUInt8),
	ENTRY(writeUInt16),
	ENTRY(writeUInt32),
	ENTRY(writeUInt64),
	ENTRY(writeFloat),
	ENTRY(writeDouble),
	ENTRY(writeString),
	ENTRY(writeBlob),
	
	ENTRY(readInt8),
	ENTRY(readInt16),
	ENTRY(readInt32),
	ENTRY(readInt64),
	ENTRY(readUInt8),
	ENTRY(readUInt16),
	ENTRY(readUInt32),
	ENTRY(readUInt64),
	ENTRY(readFloat),
	ENTRY(readDouble),
	ENTRY(readString),
	ENTRY(readBlob),
	ENTRY(haode),
	ENTRY(readbytearray),
#undef ENTRY
    {NULL, NULL}
};

LUALIB_API int luaopen_memstream(lua_State *L)
{
    #if LUA_VERSION_NUM >= 502
        luaL_newlib(L, memstream);
    #else
        luaL_register(L, "memstream", memstream);
    #endif
    return 1;
}
