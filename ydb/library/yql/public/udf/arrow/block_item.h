#pragma once

#include <ydb/library/yql/public/udf/udf_data_type.h>
#include <ydb/library/yql/public/udf/udf_string_ref.h>
#include <ydb/library/yql/public/udf/udf_type_size_check.h>

namespace NYql::NUdf {

// ABI stable
class TBlockItem {
    enum class EMarkers : ui8 {
        Empty = 0,
        Present = 1,
    };

public:
    TBlockItem() noexcept = default;
    ~TBlockItem() noexcept = default;

    TBlockItem(const TBlockItem& value) noexcept = default;
    TBlockItem(TBlockItem&& value) noexcept = default;

    TBlockItem& operator=(const TBlockItem& value) noexcept = default;
    TBlockItem& operator=(TBlockItem&& value) noexcept = default;

    template <typename T, typename = std::enable_if_t<TPrimitiveDataType<T>::Result>>
    inline explicit TBlockItem(T value);

    inline explicit TBlockItem(bool value) {
        Raw.Simple.bool_ = value ? 1 : 0;
        Raw.Simple.Meta = static_cast<ui8>(EMarkers::Present);
    }

    inline explicit TBlockItem(TStringRef value) {
        Raw.String.Value = value.Data();
        Raw.String.Size = value.Size();
        Raw.Simple.Meta = static_cast<ui8>(EMarkers::Present);
    }

    inline explicit TBlockItem(const TBlockItem* tupleItems) {
        Raw.Tuple.Value = tupleItems;
        Raw.Simple.Meta = static_cast<ui8>(EMarkers::Present);
    }

    inline TBlockItem(ui64 low, ui64 high) {
        Raw.Halfs[0] = low;
        Raw.Halfs[1] = high;
    }

    inline ui64 Low() const {
        return Raw.Halfs[0];
    }

    inline ui64 High() const {
        return Raw.Halfs[1];
    }

    // TODO: deprecate As<T>() in favor of Get<T>()
    template <typename T, typename = std::enable_if_t<TPrimitiveDataType<T>::Result>>
    inline T As() const;

    template <typename T, typename = std::enable_if_t<TPrimitiveDataType<T>::Result>>
    inline T Get() const;

    // TODO: deprecate AsTuple() in favor of GetElements()
    inline const TBlockItem* AsTuple() const {
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present);
        return Raw.Tuple.Value;
    }

    inline const TBlockItem* GetElements() const {
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present);
        return Raw.Tuple.Value;
    }

    inline TBlockItem GetElement(ui32 index) const {
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present);
        return Raw.Tuple.Value[index];
    }

    inline TStringRef AsStringRef() const {
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present);
        return TStringRef(Raw.String.Value, Raw.String.Size);
    }

    inline TBlockItem MakeOptional() const
    {
        if (Raw.Simple.Meta)
            return *this;

        TBlockItem result(*this);
        ++result.Raw.Simple.Count;
        return result;
    }

    inline TBlockItem GetOptionalValue() const
    {
        if (Raw.Simple.Meta)
            return *this;

        Y_DEBUG_ABORT_UNLESS(Raw.Simple.Count > 0U, "Can't get value from empty.");

        TBlockItem result(*this);
        --result.Raw.Simple.Count;
        return result;
    }

    inline explicit operator bool() const { return bool(Raw); }
private:
    union TRaw {
        ui64 Halfs[2] = {0, 0};
        struct {
            union {
                #define FIELD(type) type type##_;
                PRIMITIVE_VALUE_TYPES(FIELD);
                #undef FIELD
                // According to the YQL <-> arrow type mapping convention,
                // boolean values are processed as 8-bit unsigned integer
                // with either 0 or 1 as a condition payload.
                ui8 bool_;
                ui64 Count;
            };
            union {
                ui64 Pad;
                struct {
                    ui8 Reserved[7];
                    ui8 Meta;
                };
            };
        } Simple;

        struct {
            const char* Value;
            ui32 Size;
        } String;

        struct {
            // client should know tuple size
            const TBlockItem* Value;
        } Tuple;

        EMarkers GetMarkers() const {
            return static_cast<EMarkers>(Simple.Meta);
        }

        explicit operator bool() const { return Simple.Meta | Simple.Count; }
    } Raw;
};

UDF_ASSERT_TYPE_SIZE(TBlockItem, 16);

#define VALUE_AS(xType) \
    template <> \
    inline xType TBlockItem::As<xType>() const \
    { \
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present); \
        return Raw.Simple.xType##_; \
    }

#define VALUE_GET(xType) \
    template <> \
    inline xType TBlockItem::Get<xType>() const \
    { \
        Y_DEBUG_ABORT_UNLESS(Raw.GetMarkers() == EMarkers::Present); \
        return Raw.Simple.xType##_; \
    }

#define VALUE_CONSTR(xType) \
    template <> \
    inline TBlockItem::TBlockItem(xType value) \
    { \
        Raw.Simple.xType##_ = value; \
        Raw.Simple.Meta = static_cast<ui8>(EMarkers::Present); \
    }

PRIMITIVE_VALUE_TYPES(VALUE_AS)
PRIMITIVE_VALUE_TYPES(VALUE_GET)
PRIMITIVE_VALUE_TYPES(VALUE_CONSTR)
// XXX: TBlockItem constructor with <bool> parameter is implemented above.
VALUE_AS(bool)
VALUE_GET(bool)

#undef VALUE_AS
#undef VALUE_GET
#undef VALUE_CONSTR

}
