#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// EdAsmNg Assembler Public API
// This header provides the test-facing API for the assembler implementation.
// Internal implementation details remain in src/lib/asm/asm.cpp

namespace EdAsmNg {
  namespace Asm {

    //=================================================
    // Test Setup & Source Configuration
    //=================================================

    void           ClearTestBuffer();
    void           SetupSourceLine(const char* line);
    void           SetupOperandField(const char* operand);
    void           SetupMemorySource(const char* sourceText, size_t length);
    void           RewindSource();
    std::uintptr_t GetMnemP();
    void           SetupMnemP(std::uint8_t* mnemEntry, std::uint8_t y_offset);

    //=================================================
    // Core Assembler Phases
    //=================================================

    void        DoPass1();
    void        DoPass2();
    void        DoPass3();
    std::string BuildListingOutput(const char* sourceName);

    //=================================================
    // Instruction Processing
    //=================================================

    void GInstLen();
    void StorGMC();
    bool HndlMnem();
    void GenMCode(std::uint8_t opcode);
    void GOpAdr();
    void ValidateRange();
    bool ChkRng(std::uint8_t value, std::uint8_t minVal, std::uint8_t maxVal);

    //=================================================
    // Operand & Expression Evaluation
    //=================================================

    void EvalOprnd();

    //=================================================
    // Directive Handlers
    //=================================================

    void        HndlOBJ();
    void        HndlREL();
    void        HndlDS();
    void        HndlDFB();
    void        HndlDW();
    void        HndlASC();
    void        HndlDCI();
    void        HndlLST();
    void        HndlNOLIST();
    void        DoPage();
    void        HndlSBTL();
    bool        CallDirectiveDispatch(const char* directiveName);
    const char* GetLastDirectiveCalled();

    //=================================================
    // Memory & Code Generation
    //=================================================

    void         StorByt(std::uint8_t byte);
    std::uint8_t ReadObjMemory(std::uint16_t addr);
    void         WriteObjMemory(std::uint16_t addr, std::uint8_t value);
    void         InitObjMemory();
    void         EnableTestObjMemory(bool enable);
    std::uint8_t GetTestObjMemory(std::uint16_t addr);
    void         ClearTestObjMemory();

    //=================================================
    // Instruction Encoding State
    //=================================================

    std::uint8_t GetLength();
    void         SetLength(std::uint8_t value);
    std::uint8_t GetLenTIdx();
    void         SetLenTIdx(std::uint8_t value);
    std::uint8_t GetGMC(std::uint8_t index);
    void         SetGMC(std::uint8_t index, std::uint8_t value);
    std::uint8_t GetGMCIdx();
    std::uint8_t GetZAB();
    std::uint8_t GetSubTIdx();
    void         SetSubTIdx(std::uint8_t value);

    //=================================================
    // Address & PC Management
    //=================================================

    std::uint16_t GetPC();
    void          SetPC(std::uint16_t pc);
    std::uint16_t GetObjPC();
    void          SetObjPC(std::uint16_t value);
    std::uint16_t GetCurAdr();
    void          SetCurAdr(std::uint16_t addr);
    std::uint16_t GetHighMem();
    void          SetHighMem(std::uint16_t value);

    //=================================================
    // Expression & Operand Values
    //=================================================

    std::uint16_t GetValExpr();
    void          SetValExpr(std::uint16_t value);
    std::uint8_t  GetModWrdL();
    void          SetModWrdL(std::uint8_t value);
    std::uint8_t  GetRelExprF();
    void          SetRelExprF(std::uint8_t value);

    //=================================================
    // Flags & Mode Control
    //=================================================

    std::uint8_t GetGenF();
    void         SetGenF(std::uint8_t value);
    std::uint8_t GetRelCodeF();
    void         SetRelCodeF(std::uint8_t value);
    std::uint8_t GetDummyF();
    void         SetDummyF(std::uint8_t value);
    std::uint8_t GetLabelF();
    void         SetLabelF(std::uint8_t value);
    std::uint8_t GetPassNbr();
    void         SetPassNbr(std::uint8_t pass);
    void         SetUseExperimentalPass2(bool enable);
    bool         GetUseExperimentalPass2();
    bool         GetCarryFlag();
    void         SetCarryFlag(bool value);
    bool         GetLastCarryFlag();

    //=================================================
    // Token & Parsing State
    //=================================================

    std::uint8_t  GetNxtToken();
    void          SetNxtToken(std::uint8_t value);
    std::uint8_t  GetY();
    void          SetY(std::uint8_t value);
    std::uint8_t  GetA();
    void          SetA(std::uint8_t value);
    std::uint8_t  GetSrcPByte(std::uint8_t index);
    std::uint16_t GetSrcP();
    void          SetSrcP(std::uint16_t value);
    std::uint16_t GetTxtEnd();
    void          SetTxtEnd(std::uint16_t value);
    std::int8_t   GetIDskSrcF();
    void          SetIDskSrcF(std::int8_t value);

    //=================================================
    // Symbol Table Management
    //=================================================

    void          InitSymbolTable();
    void          ClearSymbolTable();
    bool          HasSymbol(const char* name);
    std::uint16_t GetSymbolValue(const char* name);
    std::uint8_t  GetSymbolFlags(const char* name);
    int           GetSymbolCount();
    void          SetSymFBP(std::uint16_t ptr);
    std::uint16_t GetSymFBP();
    std::uint16_t GetStrtSymT();
    void          SetStrtSymT(std::uint16_t value);
    std::uint16_t GetEndSymT();
    void          SetEndSymT(std::uint16_t value);
    std::uint16_t GetHeaderT(std::uint8_t index);
    void          SetHeaderT(std::uint8_t index, std::uint16_t value);

    //=================================================
    // Memory Management
    //=================================================

    std::uint16_t GetMemTop();
    void          SetMemTop(std::uint16_t value);
    std::uint16_t GetRLDEnd();
    void          SetRLDEnd(std::uint16_t value);
    std::uint16_t GetRLDEntryCount();
    void          GetRLDEntry(int index, std::uint8_t* entry);

    //=================================================
    // Listing Control Flags
    //=================================================

    std::uint8_t GetListingF();
    void         SetListingF(std::uint8_t value);
    std::uint8_t GetLstCyc();
    void         SetLstCyc(std::uint8_t value);
    std::uint8_t GetLstUnAsm();
    void         SetLstUnAsm(std::uint8_t value);
    std::uint8_t GetLstExpMac();
    void         SetLstExpMac(std::uint8_t value);
    std::uint8_t GetLstWarns();
    void         SetLstWarns(std::uint8_t value);
    std::uint8_t GetLstGCode();
    void         SetLstGCode(std::uint8_t value);
    std::uint8_t GetLstASym();
    void         SetLstASym(std::uint8_t value);
    std::uint8_t GetLstVSym();
    void         SetLstVSym(std::uint8_t value);
    std::uint8_t GetLst6Cols();
    void         SetLst6Cols(std::uint8_t value);
    std::uint8_t GetSubTtlF();
    void         SetSubTtlF(std::uint8_t value);
    const char*  GetSubTitle();
    void         ClearSubTitle();
    std::uint8_t GetPrSlot();
    void         SetPrSlot(std::uint8_t value);

    //=================================================
    // Error & Warning Management
    //=================================================

    void SetNumErrs(std::uint16_t value);
    void SetNumWarns(std::uint16_t value);
    void ResetErrorState();

    //=================================================
    // Line Processing
    //=================================================

    std::uint16_t GetLineNum();
    void          SetLineNum(std::uint16_t value);
    void          NextRec();
    void          NxtField();
    void          ChrGot();
    void          ChrGet();
    void          GSrcLin();
    void          AdvanceToNextLine();

    //=================================================
    // Initialization & Cleanup
    //=================================================

    void SaveZP();
    void RestoreZP();
    void InitASM();
    void CleanupAsm();
    void ResetAsmState();
    void ResetPass3State();
    void ResetGOpAdrState();

    //=================================================
    // Macro & Conditional Assembly
    //=================================================

    std::uint8_t GetMacroF();
    void         SetMacroF(std::uint8_t value);
    std::uint8_t GetIfDefF();
    void         SetIfDefF(std::uint8_t value);

    //=================================================
    // Pass 3: Symbol Table & Listing
    //=================================================

    // Symbol table test helpers (from asm_test_helpers.hpp)
    void          AddTestSymbol(const char* name, std::uint16_t value, std::uint8_t flags);
    std::uint8_t* GetCompactedTable();
    std::uint16_t GetCompactedTableSize();
    bool          FindSym(const char* name);
    std::uint8_t* GetSymP();

  }  // namespace Asm
}  // namespace EdAsmNg
