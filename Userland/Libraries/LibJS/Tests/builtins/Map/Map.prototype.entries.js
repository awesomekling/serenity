test("length", () => {
    expect(Map.prototype.entries.length).toBe(0);
});

test("basic functionality", () => {
    const original = [
        ["a", 0],
        ["b", 1],
        ["c", 2],
    ];
    const a = new Map(original);
    const it = a.entries();
    // FIXME: This test should be rewritten once we have proper iteration order
    const first = it.next();
    expect(first.done).toBeFalse();
    expect(a.has(first.value[0])).toBeTrue();
    const second = it.next();
    expect(second.done).toBeFalse();
    expect(a.has(second.value[0])).toBeTrue();
    const third = it.next();
    expect(third.done).toBeFalse();
    expect(a.has(third.value[0])).toBeTrue();
    expect(it.next()).toEqual({ value: undefined, done: true });
    expect(it.next()).toEqual({ value: undefined, done: true });
    expect(it.next()).toEqual({ value: undefined, done: true });
});
