#ifndef SAMPLE_CONSTANTS_H
#define SAMPLE_CONSTANTS_H

struct SampleConstants
{
    /** The product id for the running application, found on the dev portal */
    static constexpr char ProductId[] = "";
    /** The application id for the running application, found on the dev portal */
    static constexpr char ApplicationId[] = "";
    /** The sandbox id for the running application, found on the dev portal */
    static constexpr char SandboxId[] = "";
    /** The deployment id for the running application, found on the dev portal */
    static constexpr char DeploymentId[] = "";
    /** Client id of the service permissions entry, found on the dev portal */
    static constexpr char ClientCredentialsId[] = "";
    /** Client secret for accessing the set of permissions, found on the dev portal */
    static constexpr char ClientCredentialsSecret[] = "";
    /** Game name */
    static constexpr char GameName[] = "GodotEpic Demo";
    /** Encryption key. Not used by this sample. */
    static constexpr char EncryptionKey[] = "1111111111111111111111111111111111111111111111111111111";
};

#endif // SAMPLE_CONSTANTS_H