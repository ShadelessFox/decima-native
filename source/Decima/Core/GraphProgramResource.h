#pragma once

#include "Decima/Core/DynamicLibrary.h"
#include "Decima/Core/RTTIRefObject.h"
#include "Decima/Core/RTTIHandle.h"
#include "Decima/Core/WeakPtrTarget.h"

#include "Util/Assert.h"

struct ParameterDefaultHandle {
    uint32_t FlagsAndIndex;
};

assert_size(ParameterDefaultHandle, 0x4);

struct ProgramParameter {
    String Name;
    RTTIHandle Type;
    ParameterDefaultHandle Flags;
    uint32_t UUIDHash;
    uint64_t ParameterType : 48;
    uint64_t ParameterOffset : 16;
};

assert_size(ProgramParameter, 0x28);

struct ProgramParameterValue {
    Array<uint8_t> BinaryBlob;
    pVoid Unk10;
    pVoid Unk18;
};

assert_size(ProgramParameterValue, 0x20);

struct ProgramParameterList {
    Array<ProgramParameter> Parameters;
    Array<ProgramParameterValue> DefaultBinaryValues;
    Array<Ref<RTTIRefObject>> DefaultSoftLinkedObjects;
    Array<Ref<RTTIRefObject>> DefaultHardLinkedObjects;
    Array<UUIDRef<RTTIRefObject>> DefaultUUIDRefs;
    bool Unk50;
    Array<uint32_t> Unk58;
};

assert_size(ProgramParameterList, 0x68);

struct ProgramResourceEntryPoint {
    ProgramParameterList InputParameters;
    ProgramParameterList OutputParameters;
    String EntryPoint;
    pVoid ProcAddress;
};

assert_size(ProgramResourceEntryPoint, 0xE0);

class ProgramResource : public RTTIRefObject {
public:
    // Program
    Array<ProgramResourceEntryPoint> EntryPoints;
    Ref<DynamicLibrary> Library;
    uint64_t StorageHash;
    bool CompileRuntimeDebug;
};

assert_size(ProgramResource, 0x48);

class ProgramParameterBindingsBase {
public:
    virtual ~ProgramParameterBindingsBase() = 0;
};

class ProgramParameterBindings : public ProgramParameterBindingsBase {
public:
    ~ProgramParameterBindings() override = default;

public:
    pVoid Values;
    ProgramParameterList *Parameters;
};

assert_size(ProgramParameterBindings, 0x18);

class NodeConstantsResource : public RTTIRefObject {
public:
    ProgramParameterList Parameters;
    Array<int> ExposedObjectsIndices;
    Array<int> ExposedUUIDRefIndices;
};

assert_size(NodeConstantsResource, 0xA8);

class ProgramInstance : public RTTIRefObject {
public:
    Ref<ProgramResource> Program;
    Array<ProgramParameterBindings> InputParameterBindings;
    Array<ProgramParameterBindings> OutputParameterBindings;
    pVoid Unk48;
    pVoid Unk50;
};

assert_size(ProgramInstance, 0x58);

class StateObjectInstance : public CoreObject {
public:
    Ref<RTTIRefObject> Unk20;
};

assert_size(StateObjectInstance, 0x28);

class GraphProgramInstance : public ProgramInstance, public WeakPtrRTTITarget {
public:
    Ref<NodeConstantsResource> ExposedData;
    ProgramParameterBindings ExposedDataBindings;

    Array<Ref<RTTIRefObject>> ExposedObjects;
    Array<UUIDRef<RTTIRefObject>> UUIDRefs;

    Array<Ref<StateObjectInstance>> StateObjects;
    ProgramParameterBindings StateParameterBindings;

    pVoid UnkD0;
    pVoid UnkD8;
    pVoid UnkE0;
    pVoid UnkE8;
    pVoid UnkF0;
    pVoid UnkF8;
    pVoid Unk100;
    pVoid Unk108;
};

assert_size(GraphProgramInstance, 0x110);
assert_offset(GraphProgramInstance, ExposedData, 0x68);
assert_offset(GraphProgramInstance, ExposedDataBindings, 0x70);
assert_offset(GraphProgramInstance, ExposedObjects, 0x88);
assert_offset(GraphProgramInstance, UUIDRefs, 0x98);
assert_offset(GraphProgramInstance, StateObjects, 0xA8);
assert_offset(GraphProgramInstance, StateParameterBindings, 0xB8);
