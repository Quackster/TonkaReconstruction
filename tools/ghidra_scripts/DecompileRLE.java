import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.util.*;

public class DecompileRLE extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);

        FunctionManager fm = currentProgram.getFunctionManager();

        println("=== Decompiling all functions ===");
        FunctionIterator iter = fm.getFunctions(true);
        int count = 0;
        while (iter.hasNext()) {
            Function func = iter.next();
            DecompileResults results = decomp.decompileFunction(func, 30, monitor);
            if (results.decompileCompleted()) {
                DecompiledFunction df = results.getDecompiledFunction();
                if (df != null) {
                    String code = df.getC();
                    count++;
                    if (code.length() > 100) {
                        println("\n--- Function at " + func.getEntryPoint() + " (" + func.getName() + ") ---");
                        println(code);
                    }
                }
            }
        }
        println("\nTotal functions decompiled: " + count);
        decomp.dispose();
    }
}
