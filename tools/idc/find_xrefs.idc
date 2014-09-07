#include <idc.idc>

static main()
{
    auto ea, ref;
    auto f = 0, fname = AskFile(1, "*.*", "Select a file to save the references");
    if(fname != 0 && (f = fopen(fname, "w")) != 0)
    {
        auto begin_ea = BADADDR, end_ea = BADADDR;

        auto yn = AskYN(1, "Would you like to search all xrefs to the identifier the cursor is over?");
        if(yn == 0)
        {
            begin_ea = AskAddr(0x0, "What's the begin of the data to search refs for?");
            if(begin_ea != BADADDR) end_ea = AskAddr(0x0, "What's the end of the data (exclusive) to search refs for?");
        }
        else if(yn == 1)
        {
            begin_ea = ScreenEA();
            end_ea   = begin_ea + ItemSize(begin_ea);
        }
        
        if(begin_ea != BADADDR && end_ea != BADADDR)
        {
            for(ea = begin_ea; ea < end_ea; ++ea)
            {
                // Iterate on the data range finding the xrefs
                for (ref = DfirstB(ea); ref != BADADDR; ref = DnextB(ea, ref))
                {
                    // Find the exact position of the reference (assumes little endian code)
                    auto pos = ref, inst, max_inst = NextHead(ref, BADADDR) - 3; // 'cuz of ref
                    if(max_inst != BADADDR)
                    {
                        for(inst = ref; inst < max_inst; ++inst)
                        {
                            auto v = (GetOriginalByte(inst) | (GetOriginalByte(inst+1) << 8) | (GetOriginalByte(inst+2) << 16) | (GetOriginalByte(inst+3) << 24));
                            if(v == ea) { pos = inst; break; }
                        }
                        
                        if(inst == max_inst)
                            Message("Warning: Could not find exact address of reference, this should never happen\n");
                    }
                
                    // Output it to the file
                    auto str = "0x" + ltoa(pos, 16) + " = 0x" + ltoa(begin_ea, 16) + " + 0x" + ltoa(ea - begin_ea, 16) + "  -> " + GetDisasm(ref) + "\n";
                    //Message(str);
                    fprintf(f, str);
                }
            }
            
            Message("References found successfully\n");
        }
        
        fclose(f);
    }
    else
        Message("Failed to open file to output references\n");
}
