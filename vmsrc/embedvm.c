/*
 *  EmbedVM - Embedded Virtual Machine for uC Applications
 *
 *  Copyright (C) 2011  Clifford Wolf <clifford@clifford.at>
 *  
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "embedvm.h"
// Используем часть значения(val),которая покрыта маской(mask)(required to be all zeros
//followed by all ones) as a signed value (two's complement)
//signext может выделять 3х,4х битовый аргумент как знаковый:I16 для опкодов.
//Здесь мы пытаемся посмотреть крайний старший бит значения val:UI16 за счет маски.
//Если он (1) через 'двойное дополнение мы сделаем его отрицательным(знаковым)
//Если он (0) то это будет положит число.Вообщем вернем его val. 

static inline int16_t signext(uint16_t val, uint16_t mask)
{
	val = val & mask;
	if ((val & ~(mask >> 1)) != 0)
		val |= ~mask;
	return val;
}
// extern func_ispoln1InstrVm_sPtrStrctEmbedvmRV
extern void embedvm_exec(struct embedvm_s *vm)
{   // Получаем значение:UI8 - число опкод-операции из памяти ,которая выделена в host окружении,в файле evmdemo.c 
    //посредством 'выделения' из структуры,компаненты байт-интерпритатора ip и user_ctx,которая выделена и 
    //инициализирована в том же файле - evmdemo.c
	uint8_t opcode = vm->mem_read(vm->ip, false, vm->user_ctx);
	uint16_t addr = 0;
    // операнды:I16 для арифметических,побитовых,логических операций
	int16_t a = 0, b = 0;
	int8_t sfa = 0;
    // тестируем  опкод-операции для байт-интерпритатора
	switch (opcode)
	{
	case 0x00 ... 0x3f://Проверяем принадлежност опкода к диапазону [0...63]
                           //здесь ложим локальную переменную на стек
                //число sfa:I8 E [0...63],sfa закодираван (6)-мя бит
		sfa = signext(opcode, 0x3f);
		embedvm_push(vm, embedvm_local_read(vm, sfa));
		vm->ip++;
		break;
    //Проверяем принадлежност опкода к диапазону [64...127]
    //здесь как локальную со стека   
	case 0x40 ... 0x7f:
		sfa = signext(opcode, 0x3f);
		embedvm_local_write(vm, sfa, embedvm_pop(vm));
		vm->ip++;
		break;
    //Проверяем принадлежност опкода к диапазону [128...173]       
    //<128...173>        
	case 0x80+0 ... 0x80+11:.//случай(128...139)(dec)
        //При опкоде E[128...139 U 168...173] извлеч из стека (b)    
	case 0xa8+0 ... 0xa8+5: 


		b = embedvm_pop(vm);
    //</128...173>        
    //При опкоде E(140...142) извлечь из стека только (a) для унарных операций        
	case 0x80+12 ... 0x80+14:
		a = embedvm_pop(vm);//случай (140...142)(dec) побитовое не,арифмет. инверсия,логическое не
		switch (opcode)
		{
			case 0x80 +  0: embedvm_push(vm, a + b);  break;
			case 0x80 +  1: embedvm_push(vm, a - b);  break;
			case 0x80 +  2: embedvm_push(vm, a * b);  break;
			case 0x80 +  3: embedvm_push(vm, a / b);  break;
			case 0x80 +  4: embedvm_push(vm, a % b);  break;
			case 0x80 +  5: embedvm_push(vm, a << b); break;
			case 0x80 +  6: embedvm_push(vm, a >> b); break;
			case 0x80 +  7: embedvm_push(vm, a & b);  break;
			case 0x80 +  8: embedvm_push(vm, a | b);  break;
			case 0x80 +  9: embedvm_push(vm, a ^ b);  break;
			case 0x80 + 10: embedvm_push(vm, a && b); break;
			case 0x80 + 11: embedvm_push(vm, a || b); break;
			case 0x80 + 12: embedvm_push(vm, ~a);     break;
			case 0x80 + 13: embedvm_push(vm, -a);     break;
			case 0x80 + 14: embedvm_push(vm, !a);     break;
			case 0xa8 +  0: embedvm_push(vm, a <  b); break;
			case 0xa8 +  1: embedvm_push(vm, a <= b); break;
			case 0xa8 +  2: embedvm_push(vm, a == b); break;
			case 0xa8 +  3: embedvm_push(vm, a != b); break;
			case 0xa8 +  4: embedvm_push(vm, a >= b); break;
			case 0xa8 +  5: embedvm_push(vm, a >  b); break;
			
		}
		vm->ip++;
		break;
    //Проверяем принадлежност опкода к диапазону [144...151] 
    //ложит непосредственно значение на стек  
	case 0x90 ... 0x97:
   //маска 0x07 говорит что непосредственно значение (a)
    //закодирована (3)-мя бит т е максимальное положительное значение будет число (3)
		a = signext(opcode, 0x07);
		if ((a & 0x04) != 0)
			a |= ~0x07;
		embedvm_push(vm, a);
		vm->ip++;
		break;
	case 0x98: // ложит в стек безнаковый 1 байт:UI8
		a = vm->mem_read(vm->ip+1, false, vm->user_ctx) & 0x00ff;
		embedvm_push(vm, a);
		vm->ip += 2;
		break;
	case 0x99: // ложит в стек знаковый 1 байт:I8
		a = vm->mem_read(vm->ip+1, false, vm->user_ctx) & 0x00ff;
		embedvm_push(vm, signext(a, 0x00ff));
		vm->ip += 2;
		break;
	case 0x9a:  // ложит в стек знаковый 2 байт:I16
		a = vm->mem_read(vm->ip+1, true, vm->user_ctx);
		embedvm_push(vm, a);
		vm->ip += 3;
		break;
	case 0x9b:
		a = embedvm_pop(vm);
		if (0) {
	case 0x9c:
			a = 0;
		}
		vm->sp = vm->sfp;
		vm->ip = embedvm_pop(vm);
		vm->sfp = embedvm_pop(vm);
		if ((vm->sfp & 1) != 0)
			vm->sfp &= ~1;
		else
			embedvm_push(vm, a);
		break;
	case 0x9d:
		embedvm_pop(vm);
		vm->ip++;
		break;
	case 0x9e:
		addr = embedvm_pop(vm);
		if (vm->mem_read(vm->ip+1, false, vm->user_ctx) == 0x9d) {
			embedvm_push(vm, vm->sfp | 1);
			embedvm_push(vm, vm->ip + 2);
		} else {
			embedvm_push(vm, vm->sfp);
			embedvm_push(vm, vm->ip + 1);
		}
		vm->sfp = vm->sp;
		vm->ip = addr;
		break;
	case 0x9f:
		vm->ip = embedvm_pop(vm);
		break;
	case 0xa0 ... 0xa0+7:
		if ((opcode & 1) == 0) {
			addr = vm->ip + signext(vm->mem_read(vm->ip+1, false, vm->user_ctx), 0x00ff);
			vm->ip += 2;
		} else {
			addr = vm->ip + vm->mem_read(vm->ip+1, true, vm->user_ctx);
			vm->ip += 3;
		}
		switch (opcode)
		{
		case 0xa0:
		case 0xa1:
			vm->ip = addr;
			break;
		case 0xa2:
		case 0xa3:
			if (vm->mem_read(vm->ip, false, vm->user_ctx) == 0x9d) {
				embedvm_push(vm, vm->sfp | 1);
				embedvm_push(vm, vm->ip + 1);
			} else {
				embedvm_push(vm, vm->sfp);
				embedvm_push(vm, vm->ip);
			}
			vm->sfp = vm->sp;
			vm->ip = addr;
			break;
		case 0xa4:
		case 0xa5:
			if (embedvm_pop(vm))
				vm->ip = addr;
			break;
		case 0xa6:
		case 0xa7:
			if (!embedvm_pop(vm))
				vm->ip = addr;
			break;
		}
		break;
	case 0xae:
		embedvm_push(vm, vm->sp);
		vm->ip++;
		break;
	case 0xaf:
		embedvm_push(vm, vm->sfp);
		vm->ip++;
		break;
	case 0xb0 ... 0xb0+15:
		{
			uint8_t argc = embedvm_pop(vm);
			int16_t argv[argc];
			for (sfa=0; sfa<argc; sfa++)
				argv[sfa] = embedvm_pop(vm);
			a = vm->call_user(opcode - 0xb0, argc, argv, vm->user_ctx);
			embedvm_push(vm, a);
		}
		vm->ip++;
		break;
	case 0xc0 ... 0xef:
		if ((opcode & 0x07) == 5) {
			/* this is a "bury" instruction */
			uint8_t depth = (opcode >> 3) & 0x07;
			int16_t stack[depth+1];
			for (sfa = 0; sfa <= depth; sfa++)
				stack[sfa] = embedvm_pop(vm);
			embedvm_push(vm, stack[0]);
			for (sfa = depth; sfa > 0; sfa--)
				embedvm_push(vm, stack[sfa]);
			embedvm_push(vm, stack[0]);
			vm->ip++;
			break;
		}
		if ((opcode & 0x07) == 6) {
			/* this is a "dig" instruction */
			uint8_t depth = (opcode >> 3) & 0x07;
			int16_t stack[depth+2];
			for (sfa = 0; sfa < depth+2; sfa++)
				stack[sfa] = embedvm_pop(vm);
			for (sfa = depth+1; sfa > 0; sfa--)
				embedvm_push(vm, stack[sfa-1]);
			embedvm_push(vm, stack[depth+1]);
			vm->ip++;
			break;
		}
		sfa = ((opcode >> 3) & 0x07) == 4 || ((opcode >> 3) & 0x07) == 5 ? 1 : 0;
		switch (opcode & 0x07)
		{
		case 0:
			addr = vm->mem_read(vm->ip+1, false, vm->user_ctx) & 0x00ff;
			vm->ip += 2;
			break;
		case 1:
			addr = vm->mem_read(vm->ip+1, true, vm->user_ctx);
			vm->ip += 3;
			break;
		case 2:
			addr = embedvm_pop(vm);
			vm->ip++;
			break;
		case 3:
			addr = (embedvm_pop(vm) << sfa) + (vm->mem_read(vm->ip+1, false, vm->user_ctx) & 0x00ff);
			vm->ip += 2;
			break;
		case 4:
			addr = (embedvm_pop(vm) << sfa) + vm->mem_read(vm->ip+1, true, vm->user_ctx);
			vm->ip += 3;
			break;
		}
		switch ((opcode >> 3) & 0x07)
		{
		case 0:
			embedvm_push(vm, vm->mem_read(addr, false, vm->user_ctx) & 0x00ff);
			break;
		case 1:
			vm->mem_write(addr, embedvm_pop(vm), false, vm->user_ctx);
			break;
		case 2:
			embedvm_push(vm, signext(vm->mem_read(addr, false, vm->user_ctx), 0x00ff));
			break;
		case 3:
			vm->mem_write(addr, embedvm_pop(vm), false, vm->user_ctx);
			break;
		case 4:
			embedvm_push(vm, vm->mem_read(addr, true, vm->user_ctx));
			break;
		case 5:
			vm->mem_write(addr, embedvm_pop(vm), true, vm->user_ctx);
			break;
		}
		break;
	case 0xf0 ... 0xf7:
		for (sfa = 0; sfa <= (opcode & 0x07); sfa++)
			embedvm_push(vm, 0);
		vm->ip++;
		break;
	case 0xf8 ... 0xff:
		a = embedvm_pop(vm);
		vm->sp += 2 + 2*(opcode & 0x07);
		embedvm_push(vm, a);
		vm->ip++;
		break;
	}
}

void embedvm_interrupt(struct embedvm_s *vm, uint16_t addr)
{
	embedvm_push(vm, vm->sfp | 1);
	embedvm_push(vm, vm->ip);
	vm->sfp = vm->sp;
	vm->ip = addr;
}
// func_staskivaetSoSteksVm_sPtrStrctEmbedvmRI2
// f принимает компанент байт-интерпритатора и 'вытскивает' оттуда sp, user_ctx
//снимает со стека
int16_t embedvm_pop(struct embedvm_s *vm)
{    
    // по адресу выраженному через sp получаем значение - это значение элемента с вершины стека:I16
	int16_t value = vm->mem_read(vm->sp, true, vm->user_ctx);
	vm->sp += 2;
	return value;
}
// func_zatalkivaetNaStekZnach_sPtrStrctEmbedvmI2RI2
// f принимает компанент байт-интерпритатора и 'вытскивает' оттуда sp, user_ctx
//ложит на стек
void embedvm_push(struct embedvm_s *vm, int16_t value)
{
	vm->sp -= 2;
    // по адресу addr выраженному через sp пишем значение:I16
	vm->mem_write(vm->sp, value, true, vm->user_ctx);
}

int16_t embedvm_local_read(struct embedvm_s *vm, int8_t sfa)
{
	uint16_t addr = vm->sfp - 2*sfa + (sfa < 0 ? +2 : -2);
	return vm->mem_read(addr, true, vm->user_ctx);
}

void embedvm_local_write(struct embedvm_s *vm, int8_t sfa, int16_t value)
{
	uint16_t addr = vm->sfp - 2*sfa + (sfa < 0 ? +2 : -2);
	vm->mem_write(addr, value, true, vm->user_ctx);
}

