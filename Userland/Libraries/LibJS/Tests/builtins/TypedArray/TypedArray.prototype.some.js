const TYPED_ARRAYS = [
    Uint8Array,
    Uint16Array,
    Uint32Array,
    Int8Array,
    Int16Array,
    Int32Array,
    Float32Array,
    Float64Array,
];

const BIGINT_TYPED_ARRAYS = [BigUint64Array, BigInt64Array];

test("length is 1", () => {
    TYPED_ARRAYS.forEach(T => {
        expect(T.prototype.some).toHaveLength(1);
    });

    BIGINT_TYPED_ARRAYS.forEach(T => {
        expect(T.prototype.some).toHaveLength(1);
    });
});

describe("errors", () => {
    function errorTests(T) {
        test(`requires at least one argument (${T.name})`, () => {
            expect(() => {
                new T().some();
            }).toThrowWithMessage(
                TypeError,
                "TypedArray.prototype.some() requires at least one argument"
            );
        });

        test(`callback must be a function (${T.name})`, () => {
            expect(() => {
                new T().some(undefined);
            }).toThrowWithMessage(TypeError, "undefined is not a function");
        });
    }

    TYPED_ARRAYS.forEach(T => errorTests(T));
    BIGINT_TYPED_ARRAYS.forEach(T => errorTests(T));
});

test("basic functionality", () => {
    TYPED_ARRAYS.forEach(T => {
        const typedArray = new T([2, 4, 6]);
        expect(typedArray.some(value => value === 2)).toBeTrue();
        expect(typedArray.some(value => value === 4)).toBeTrue();
        expect(typedArray.some(value => value === 6)).toBeTrue();
        expect(typedArray.some(value => value % 2 === 0)).toBeTrue();
        expect(typedArray.some(value => value % 2 === 1)).toBeFalse();
        expect(typedArray.some(value => value < 2)).toBeFalse();
        expect(typedArray.some(value => value > 2)).toBeTrue();
    });

    BIGINT_TYPED_ARRAYS.forEach(T => {
        const typedArray = new T([2n, 4n, 6n]);
        expect(typedArray.some(value => value === 2n)).toBeTrue();
        expect(typedArray.some(value => value === 4n)).toBeTrue();
        expect(typedArray.some(value => value === 6n)).toBeTrue();
        expect(typedArray.some(value => value % 2n === 0n)).toBeTrue();
        expect(typedArray.some(value => value % 2n === 1n)).toBeFalse();
        expect(typedArray.some(value => value < 2n)).toBeFalse();
        expect(typedArray.some(value => value > 2n)).toBeTrue();
    });
});
