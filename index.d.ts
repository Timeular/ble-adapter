export interface Adapter {
    name: string;
    manufacturer: string;
    bleCapable: boolean;
    isConnected: boolean;
    hasProblem: boolean;
}

export function list(): Array<Adapter>;
