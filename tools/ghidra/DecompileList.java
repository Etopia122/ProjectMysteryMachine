// Headless Ghidra script: decompiles the functions named in a list file and
// writes one <name>.c file per function, as a first C draft for matching.
//
// Script arguments: <function list file> <output dir>
//   The list file has one function name per line (blank lines and lines
//   starting with '#' are ignored). A name may also be "0x80112154" to pick a
//   function by address.
//
// Run through tools/ghidra_decomp.py, which wraps analyzeHeadless.
//@category Scoob

import java.io.File;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolType;

public class DecompileList extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("usage: DecompileList <list file> <output dir>");
            return;
        }
        List<String> names = Files.readAllLines(new File(args[0]).toPath());
        File outDir = new File(args[1]);
        outDir.mkdirs();

        DecompInterface decomp = new DecompInterface();
        DecompileOptions options = new DecompileOptions();
        decomp.setOptions(options);
        decomp.toggleCCode(true);
        decomp.toggleSyntaxTree(true);
        decomp.setSimplificationStyle("decompile");
        if (!decomp.openProgram(currentProgram)) {
            printerr("decompiler failed to open program: " + decomp.getLastMessage());
            return;
        }

        int done = 0;
        for (String raw : names) {
            String name = raw.trim();
            if (name.isEmpty() || name.startsWith("#")) {
                continue;
            }
            Function f = findFunction(name);
            if (f == null) {
                printerr("not found: " + name);
                continue;
            }
            DecompileResults res = decomp.decompileFunction(f, 120, monitor);
            String safe = name.replaceAll("[^A-Za-z0-9_]", "_");
            try (PrintWriter w = new PrintWriter(new File(outDir, safe + ".c"), "UTF-8")) {
                w.println("// " + f.getName() + " @ " + f.getEntryPoint());
                if (res != null && res.decompileCompleted()) {
                    w.print(res.getDecompiledFunction().getC());
                } else {
                    w.println("// decompile failed: " + (res == null ? "null" : res.getErrorMessage()));
                }
            }
            done++;
        }
        decomp.dispose();
        println("decompiled " + done + " functions to " + outDir);
    }

    private Function findFunction(String name) {
        if (name.startsWith("0x")) {
            Address a = toAddr(Long.parseLong(name.substring(2), 16));
            Function f = getFunctionAt(a);
            if (f == null) {
                f = createFunction(a, null);
            }
            return f;
        }
        for (Symbol s : currentProgram.getSymbolTable().getSymbols(name)) {
            if (s.getSymbolType() == SymbolType.FUNCTION) {
                return getFunctionAt(s.getAddress());
            }
            Function f = getFunctionAt(s.getAddress());
            if (f != null) {
                return f;
            }
        }
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            if (f.getName().equals(name)) {
                return f;
            }
        }
        return null;
    }
}
