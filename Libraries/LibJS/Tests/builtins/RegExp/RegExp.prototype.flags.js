test("basic functionality", () => {
    expect(/foo/.flags).toBe("");
    expect(/foo/g.flags).toBe("g");
    expect(/foo/i.flags).toBe("i");
    expect(/foo/m.flags).toBe("m");
    expect(/foo/s.flags).toBe("s");
    expect(/foo/u.flags).toBe("u");
    expect(/foo/y.flags).toBe("y");
    expect(/foo/sgimyu.flags).toBe("gimsuy");
});
