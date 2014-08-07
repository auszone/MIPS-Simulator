//version april 12 10:00 pm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
bool cycle(unsigned int);
int cycleCount;
unsigned int *iMemory, *dMemory, regs[32], cPC, iPC, iNum, dNum, iSP;
FILE *snapshot, *error;
void prSnapshot(FILE *fPtr, int cycleCount)
{
	fprintf(fPtr, "cycle %d\n", cycleCount);
	for(int i = 0; i < 32; i++){
		fprintf(fPtr, "$%02d: 0x%08X\n", i, regs[i]);
	}
	fprintf(fPtr, "PC: 0x%08X\n\n\n", cPC);
}
//error detecting
bool writeReg0(int numReg)
{
    if (numReg == 0) {
        fprintf(error, "Write $0 error in cycle: %d\n", cycleCount);
        return true;
    }else return false;
}
bool numOverflow(int a, int b)
{
    if(a > 0 && b > 0){
        if((a + b) <= 0){
            fprintf(error, "Number overflow in cycle: %d\n", cycleCount);
            return true;
        }
    }else if(a < 0 && b < 0){
        if((a + b) >= 0){
            fprintf(error, "Number overflow in cycle: %d\n", cycleCount);
            return true;
        }
    }
    return  false;
}
bool addOverflow(int address)
{
    if(address > 1023 || address < 0){
        fprintf(error, "Address overflow in cycle: %d\n", cycleCount);
        return true;
    }
    else
        return false;
}
bool misAlign(int c, int base)
{
    if(c%base != 0){
        fprintf(error, "Misalignment error in cycle: %d\n", cycleCount);
        return true;
    }
    return false;
}
//error detecting end
int main(int argc, char const *argv[])
{
	FILE *iImage = fopen("iimage.bin", "rb");
	FILE *dImage = fopen("dimage.bin", "rb");
	snapshot = fopen("snapshot.rpt", "w");
    error = fopen("error_dump.rpt", "w");
	fread(&iPC, sizeof(int), 1, iImage);
	cPC = iPC;
	fread(&iNum, sizeof(int), 1, iImage);
	//read iImage to iMemory
	iMemory = (unsigned int*)malloc(1024);
    for (int i = 0; i < 128;  i++) {
        iMemory[i] = 0;
    }
	fread(iMemory, sizeof(int), iNum, iImage);
	//read dImage to dMemory
	fread(&regs[29], sizeof(int), 1, dImage);
	iSP = regs[29];
	fread(&dNum, sizeof(int), 1, dImage);
	dMemory = (unsigned int*)malloc(1024);
    for (int i = 0; i < 128;  i++) {
        dMemory[i] = 0;
    }
	fread(dMemory, sizeof(int), dNum, dImage);
	//execution starts here
   /* printf("IMeomory\n");
	for (int i = 0; i<iNum;i++) {
        printf("%d: %08x\n", i, iMemory[i]);
    }
    printf("DMeomory\n");
	for (int i = 0; i<dNum;i++) {
        printf("%d: %08x\n", i, dMemory[i]);
    }*/
    cycleCount = 0;
    while(1){
		//printf("Cycle:%d instruction:%08x\n", cycleCount, iMemory[(cPC-iPC)/4]);
		prSnapshot(snapshot, cycleCount);
		cycleCount++;
        if(addOverflow(cPC)) break;
		if(cycle(iMemory[(cPC-iPC)/4]))
			break;
	}
	fclose(iImage);
	fclose(dImage);
	fclose(snapshot);
	return 0;
}
bool cycle(unsigned int instr)
{
	int opCode = instr>>26;
	//printf("OpCode = %02x\n", opCode);
	if(opCode == 0){
		unsigned int funct = (instr<<26)>>26;
		unsigned int rs = (instr<<6)>>27;
		unsigned int rt = (instr<<11)>>27;
		unsigned int rd = (instr<<16)>>27;
		unsigned int shamt = (instr<<21)>>27;
        //printf("Funct= %02x\n", funct);
		switch(funct){
			case 0x20:
                if(writeReg0(rd)){
                	numOverflow(regs[rs], regs[rt]);
                	break;
                }
                numOverflow(regs[rs], regs[rt]);
				regs[rd] = regs[rs] + regs[rt];
				break;
			case 0x22:
                if(writeReg0(rd)){
                	numOverflow(regs[rs], regs[rt]);
                	break;
                }
                numOverflow(regs[rs], -regs[rt]);
				regs[rd] = regs[rs] - regs[rt];
				break;
			case 0x24:
                if(writeReg0(rd)) break;
				regs[rd] = regs[rs] & regs[rt];
				break;
			case 0x25:
                if(writeReg0(rd)) break;
				regs[rd] = regs[rs] | regs[rt];
				break;
			case 0x26:
                if(writeReg0(rd)) break;
				regs[rd] = regs[rs] ^ regs[rt];
				break;
			case 0x27:
                if(writeReg0(rd)) break;
				regs[rd] = ~(regs[rs] | regs[rt]);
				break;
			case 0x28:
                if(writeReg0(rd)) break;
				regs[rd] = ~(regs[rs] & regs[rt]);
				break;
			case 0x2A:
                if(writeReg0(rd)) break;
				regs[rd] = ((signed int)regs[rs] < (signed int)regs[rt]);
				break;
			case 0x00:
                if(writeReg0(rd)) break;
				regs[rd] = regs[rt] << shamt;
				break;
			case 0x02:
                if(writeReg0(rd)) break;
				regs[rd] = regs[rt] >> shamt;
				break;
			case 0x03:
                if(writeReg0(rd)) break;
				regs[rd] = (signed int)regs[rt] >> shamt;
				break;
			case 0x08:
				cPC = regs[rs];
                return false;
				break;
		}
		cPC += 4;
	}else{
		//halt
		if(opCode == 0x3F) return true;
		//J-type
		unsigned short int C = instr & 0x03FFFFFF;
		if(opCode == 0x02){
			cPC = (((cPC+4)>>26)<<26)|(4*C);
		}else if(opCode == 0x03){
			regs[31] = cPC+4;
			cPC = (((cPC+4)>>26)<<26)|(4*C);
		}else{
			//I-type
			unsigned int rs = (instr<<6)>>27;
			unsigned int rt = (instr<<11)>>27;
			C = (short)(instr & 0x0000FFFF);
            int wordOffset, byteOffset;
			switch(opCode){
				case 0x08:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                	numOverflow(regs[rs], C);
					regs[rt] = regs[rs] + (signed short)C;
					break;
				case 0x23:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                    numOverflow(regs[rs], C);
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
                    if(misAlign(C + regs[rs], 4)) return true;
					regs[rt] = dMemory[(regs[rs]+(signed short)C)/4];
					break;
				case 0x21:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                    numOverflow(regs[rs], C);
                    //half word . 2bytes
                    wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
                    if(misAlign(C + regs[rs], 2)) return true;
					regs[rt] = ((signed short)((dMemory[(regs[rs]/4)+wordOffset] & (0x0000FFFF << (8*byteOffset)))>>(8*byteOffset)));
					break;
				case 0x25:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                    numOverflow(regs[rs], C);
                    wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
                    if(misAlign(C + regs[rs], 2)) return true;
					regs[rt] = ((unsigned short)((dMemory[(regs[rs]/4)+wordOffset] & (0x0000FFFF << (8*byteOffset)))>>(8*byteOffset)));
                    break;
				case 0x20:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                    numOverflow(regs[rs], C);
                    //byte
                    wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
					regs[rt] = ((int8_t)((dMemory[(regs[rs]/4)+wordOffset] & (0x000000FF << (8*byteOffset)))>>(8*byteOffset)));
					break;
				case 0x24:
                    if(writeReg0(rt)) {
                    	numOverflow(regs[rs], C);
                   		break;
                	}	
                    numOverflow(regs[rs], C);
					wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
					regs[rt] = ((uint8_t)((dMemory[(regs[rs]/4)+wordOffset] & (0x000000FF << (8*byteOffset)))>>(8*byteOffset)));
					break;
				case 0x2B:
                    // word
                    numOverflow(regs[rs], C);
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
                    if(misAlign(C + regs[rs], 4)) return true;
					dMemory[(regs[rs]+(signed short)C)/4] = regs[rt];
					break;
				case 0x29:
                    numOverflow(regs[rs], C);
                    //half word . 2bytes
                    if(C % 2 != 0) return  true;
                    wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
                    if(misAlign(C + regs[rs], 2)) return true;
                    dMemory[(regs[rs]/4)+wordOffset] &= (0xFFFF0000 >> byteOffset*8);
					dMemory[(regs[rs]/4)+wordOffset] |= ((regs[rt] & 0x0000FFFF)<<byteOffset*8);
					break;
				case 0x28:
                    numOverflow(regs[rs], C);
                    //1 byte
                    wordOffset = C/4;
                    byteOffset = C%4;
                    if(addOverflow((regs[rs]+(signed short)C))) return true;
					dMemory[(regs[rs]/4)+wordOffset] |= ((regs[rt] & 0x000000FF)<<byteOffset*8);
					break;
				case 0x0F:
                    if(writeReg0(rt)) break;
					regs[rt] = C << 16;
					break;
				case 0x0C:
                    if(writeReg0(rt)) break;
					regs[rt] = regs[rs] & C;
					break;
				case 0x0D:
                    if(writeReg0(rt)) break;
					regs[rt] = regs[rs] | C;
					break;
				case 0x0E:
                    if(writeReg0(rt)) break;
					regs[rt] = ~(regs[rs] | C);
					break;
				case 0x0A:
                    if(writeReg0(rt)) break;
					regs[rt] = ((signed int)regs[rs] < (signed short) C);
					break;
				case 0x04:
					if(regs[rs] == regs[rt]){
                        numOverflow(cPC, (4*(int)C));
						cPC = cPC + (signed short)(4*C);
                    }
					break;
				case 0x05:
					if(regs[rs] != regs[rt]){
                        numOverflow(cPC, (4*(int)C));
						cPC = cPC + (signed short)(4*C);
                    }
                    break;
			}
			cPC += 4;
		}
	}
	return false;
}
